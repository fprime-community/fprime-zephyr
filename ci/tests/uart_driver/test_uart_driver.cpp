// Host-side tests for Zephyr::ZephyrUartDriver receive path
//
// Compiles the real ZephyrUartDriver.cpp against mocked Zephyr/F Prime headers
// (see mocks/) and verifies ISR back-pressure, bounded ordered draining,
// allocator-exhaustion handling, overrun reporting and single-consumer
// selection between the rate group and the optional RX task.

#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.hpp"
#include "zephyr_mocks.hpp"

#include <Fw/Logger/Logger.hpp>

#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <vector>

int Fw::Logger::s_logCount = 0;
Os::Task::Status Os::Task::s_nextStartStatus = Os::Task::OP_OK;
Os::Task::taskRoutine Os::Task::s_routine = nullptr;
void* Os::Task::s_routineArg = nullptr;
FwTaskPriorityType Os::Task::s_priority = 0;
FwSizeType Os::Task::s_stackSize = 0;
int Os::Task::s_startCount = 0;
int Os::Task::s_joinCount = 0;

static int g_failures = 0;
#define CHECK(cond)                                                                          \
    do {                                                                                     \
        if (!(cond)) {                                                                       \
            std::fprintf(stderr, "  CHECK failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
            g_failures++;                                                                    \
        }                                                                                    \
    } while (0)

using Zephyr::ZephyrUartDriver;
namespace Cfg = Zephyr::ZephyrUartDriverCfg;

static const struct device g_dev = {"uart_mock"};

static std::vector<uint8_t> pattern(size_t n, uint8_t seed = 0) {
    std::vector<uint8_t> v(n);
    for (size_t i = 0; i < n; i++) {
        v[i] = static_cast<uint8_t>(seed + i * 7 + (i >> 8));
    }
    return v;
}

static void resetAll() {
    g_uart.reset();
    g_sem.reset();
    Os::Task::s_nextStartStatus = Os::Task::OP_OK;
    Os::Task::s_routine = nullptr;
    Os::Task::s_routineArg = nullptr;
    Os::Task::s_startCount = 0;
    Os::Task::s_joinCount = 0;
    Fw::Logger::s_logCount = 0;
}

static void test_configure_enables_rx_and_signals_ready() {
    std::puts(
        "configure: registers callback, enables RX IRQ, disables TX IRQ, "
        "signals ready");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(g_uart.callback == &ZephyrUartDriver::serial_cb);
    CHECK(g_uart.userData == &drv);
    CHECK(g_uart.configuredBaud == 115200);
    CHECK(g_uart.rxIrqEnabled);
    CHECK(!g_uart.txIrqEnabled);
    CHECK(drv.h_readyCount == 1);
    CHECK(g_sem.initCount == 1);
}

static void test_configure_not_ready_device() {
    std::puts(
        "configure: device not ready -> no callback, no ready, "
        "schedIn/send are no-ops");
    resetAll();
    g_uart.deviceReady = false;
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(g_uart.callback == nullptr);
    CHECK(drv.h_readyCount == 0);
    CHECK(Fw::Logger::s_logCount == 1);
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_allocSizes.empty());
    U8 byte = 0x55;
    Fw::Buffer b(&byte, 1);
    CHECK(drv.send_handler_public(0, b) == Drv::ByteStreamStatus::OTHER_ERROR);
}

static void test_full_ring_drained_in_one_tick() {
    std::puts(
        "schedIn: a full ring is delivered in one tick, in order, in <= "
        "RX_CHUNK_SIZE chunks");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    const std::vector<uint8_t> in = pattern(Cfg::RX_RING_SIZE);
    g_uart.rxFifo = in;
    g_uart.pumpIsr();
    CHECK(g_uart.rxFifo.empty());  // whole ring filled
    CHECK(!g_uart.rxIrqEnabled);   // ring full: paused until drained
    drv.schedIn_handler_public(0, 0);
    CHECK(g_uart.rxIrqEnabled);
    CHECK(drv.h_recvStream == in);
    CHECK(drv.h_recv.size() == Cfg::RX_RING_SIZE / Cfg::RX_CHUNK_SIZE);
    for (const auto& r : drv.h_recv) {
        CHECK(r.data.size() <= Cfg::RX_CHUNK_SIZE);
        CHECK(r.status == Drv::ByteStreamStatus::OP_OK);
    }
    CHECK(drv.h_outstanding == 0);
    CHECK(drv.h_tlmRxBytes == Cfg::RX_RING_SIZE);
    CHECK(drv.h_tlmRxOverrun == 0);
    CHECK(drv.h_rxOverrunEvents.empty());
}

static void test_backpressure_no_loss() {
    std::puts(
        "ISR: ring full -> RX IRQ disabled, no bytes lost; drain "
        "re-enables and completes byte-perfect");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    const size_t total = 3 * Cfg::RX_RING_SIZE + 123;
    const std::vector<uint8_t> in = pattern(total, 3);
    g_uart.rxFifo = in;
    g_uart.rxReadLimit = 16;  // device FIFO depth: many small ISR reads
    g_uart.pumpIsr();
    CHECK(!g_uart.rxIrqEnabled);                               // paused
    CHECK(g_uart.rxFifo.size() == total - Cfg::RX_RING_SIZE);  // nothing discarded
    CHECK(g_uart.rxDisableCount == 1);
    // Each tick drains, which re-enables RX; the "device" then raises more interrupts
    for (int tick = 0; tick < 8 && drv.h_recvStream.size() < total; tick++) {
        drv.schedIn_handler_public(0, 0);
        g_uart.pumpIsr();
    }
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recvStream == in);
    CHECK(g_uart.rxIrqEnabled);
    CHECK(drv.h_tlmRxBackpressure >= 1);
    CHECK(drv.h_tlmRxOverrun == 0);
    CHECK(drv.h_outstanding == 0);
}

static void test_alloc_failure_keeps_data() {
    std::puts(
        "drain: allocator exhaustion leaves data in the ring, returns no "
        "empty buffer, retries later");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    const std::vector<uint8_t> in = pattern(600, 9);
    g_uart.rxFifo = in;
    g_uart.pumpIsr();
    drv.h_allocFail = true;
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recv.empty());
    CHECK(drv.h_deallocCount == 0);
    CHECK(drv.h_emptyDeallocCount == 0);
    CHECK(drv.h_tlmRxAllocFail == 1);
    drv.h_allocFail = false;
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recvStream == in);
    CHECK(drv.h_tlmRxAllocFail == 1);
}

static void test_alloc_failure_while_paused_keeps_backpressure() {
    std::puts(
        "drain: allocator exhaustion while paused keeps RX IRQ disabled "
        "(back-pressure propagates)");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    g_uart.rxFifo = pattern(2 * Cfg::RX_RING_SIZE);
    g_uart.pumpIsr();
    CHECK(!g_uart.rxIrqEnabled);
    drv.h_allocFail = true;
    drv.schedIn_handler_public(0, 0);
    CHECK(!g_uart.rxIrqEnabled);
    CHECK(g_uart.rxFifo.size() == Cfg::RX_RING_SIZE);
}

static void test_overrun_reported_once_per_change() {
    std::puts(
        "ISR: hardware overrun counted; event emitted from schedIn only "
        "when the count changes");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    g_uart.rxFifo = pattern(10);
    g_uart.pendingErrors = UART_ERROR_OVERRUN | UART_ERROR_FRAMING;
    g_uart.pumpIsr();
    drv.schedIn_handler_public(0, 0);
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_rxOverrunEvents.size() == 1);
    CHECK(drv.h_rxOverrunEvents[0] == 1);
    CHECK(drv.h_tlmRxOverrun == 1);
    g_uart.rxFifo = pattern(10);
    g_uart.pendingErrors = UART_ERROR_PARITY;  // not an overrun
    g_uart.pumpIsr();
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_rxOverrunEvents.size() == 1);
}

static ZephyrUartDriver* g_taskDriver = nullptr;
static void stopAfterSecondTake(int takeCount) {
    if (takeCount >= 2 && g_taskDriver != nullptr) {
        g_taskDriver->stop();
    }
}

static void test_start_makes_task_sole_consumer() {
    std::puts(
        "start: RX task is the only consumer; schedIn only nudges the "
        "semaphore and reports");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(drv.start(7, 4096) == Os::Task::OP_OK);
    CHECK(Os::Task::s_startCount == 1);
    CHECK(Os::Task::s_priority == 7);
    CHECK(Os::Task::s_stackSize == 4096);
    CHECK(Os::Task::s_routine != nullptr);
    CHECK(Os::Task::s_routineArg == &drv);

    const std::vector<uint8_t> in = pattern(700, 5);
    g_uart.rxFifo = in;
    const int givesBefore = g_sem.giveCount;
    g_uart.pumpIsr();
    CHECK(g_sem.giveCount > givesBefore);  // ISR wakes the task

    // Rate group must not read the ring
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recv.empty());
    CHECK(drv.h_allocSizes.empty());
    CHECK(drv.h_tlmRxBytes == 700);  // but still reports

    // Run the task loop inline: first take drains, second take stops the loop
    g_taskDriver = &drv;
    g_sem.onTake = stopAfterSecondTake;
    Os::Task::s_routine(Os::Task::s_routineArg);
    g_sem.onTake = nullptr;
    g_taskDriver = nullptr;
    CHECK(drv.h_recvStream == in);
    CHECK(drv.join() == Os::Task::OP_OK);
    CHECK(Os::Task::s_joinCount == 1);
}

static void test_start_failure_falls_back_to_sched() {
    std::puts("start: task creation failure logs and keeps the rate-group drain");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    Os::Task::s_nextStartStatus = Os::Task::ERROR_RESOURCES;
    CHECK(drv.start(7, 4096) == Os::Task::ERROR_RESOURCES);
    CHECK(Fw::Logger::s_logCount == 1);
    const std::vector<uint8_t> in = pattern(100, 1);
    g_uart.rxFifo = in;
    g_uart.pumpIsr();
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recvStream == in);
    CHECK(drv.join() == Os::Task::OP_OK);
    CHECK(Os::Task::s_joinCount == 0);
}

static void test_start_without_configure_returns_status() {
    std::puts(
        "start: no configured device (configure skipped or device not "
        "ready) returns a status, no task");
    resetAll();
    g_uart.deviceReady = false;
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(drv.start(7, 4096) == Os::Task::INVALID_HANDLE);
    CHECK(Os::Task::s_startCount == 0);
    CHECK(Fw::Logger::s_logCount == 2);  // not-ready + start refused
    drv.schedIn_handler_public(0, 0);    // still harmless
    CHECK(drv.h_recv.empty());
    CHECK(drv.join() == Os::Task::OP_OK);
}

static void test_configure_logs_missing_irq_api() {
    std::puts(
        "configure: device without interrupt-driven API is logged, send "
        "still works");
    resetAll();
    g_uart.callbackSetResult = -ENOSYS;
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(Fw::Logger::s_logCount == 1);
    CHECK(g_uart.callback == nullptr);
    CHECK(drv.h_readyCount == 1);
    U8 byte = 0x5A;
    Fw::Buffer b(&byte, 1);
    CHECK(drv.send_handler_public(0, b) == Drv::ByteStreamStatus::OP_OK);
}

static void refillRingFromRecv() {
    // The "device" keeps delivering while the drain runs: refill the ring after every chunk
    g_uart.rxFifo = pattern(Cfg::RX_CHUNK_SIZE, 11);
    g_uart.pumpIsr();
}

static void test_drain_bounded_under_continuous_refill() {
    std::puts(
        "drain: a ring refilled during the drain is bounded by "
        "MAX_DRAIN_ITERATIONS and leaves data for the next tick");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    g_uart.rxFifo = pattern(Cfg::RX_RING_SIZE);
    g_uart.pumpIsr();
    drv.h_onRecv = refillRingFromRecv;
    drv.schedIn_handler_public(0, 0);
    const size_t maxIterations = (2 * Cfg::RX_RING_SIZE) / Cfg::RX_CHUNK_SIZE + 1;
    CHECK(drv.h_recv.size() == maxIterations);
    CHECK(drv.h_recvStream.size() == maxIterations * Cfg::RX_CHUNK_SIZE);
    // Every chunk drained was replaced, so a full ring is still pending for the next tick
    drv.h_onRecv = nullptr;
    const size_t before = drv.h_recvStream.size();
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_recvStream.size() == before + Cfg::RX_RING_SIZE);
    CHECK(g_uart.rxIrqEnabled);
    CHECK(drv.h_tlmRxOverrun == 0);
    CHECK(drv.h_outstanding == 0);
}

static void test_send_queues_frame_isr_transmits() {
    std::puts("send: frame queued, ISR transmits it in order through a small device FIFO");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    std::vector<uint8_t> frame = pattern(300, 2);
    Fw::Buffer b(frame.data(), frame.size());
    CHECK(drv.send_handler_public(0, b) == Drv::ByteStreamStatus::OP_OK);
    CHECK(g_uart.txIrqEnabled);
    CHECK(g_uart.txOut.empty());  // nothing written on the caller's thread
    g_uart.transmitAll();
    CHECK(g_uart.txOut == frame);
    CHECK(!g_uart.txIrqEnabled);  // TX interrupt silenced once the ring is empty
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_tlmTxBytes == 300);
    CHECK(drv.h_tlmTxDrops == 0);
    CHECK(drv.h_txDropEvents.empty());
}

static void test_send_multiple_frames_in_order() {
    std::puts("send: several queued frames leave the device back to back, in order");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    std::vector<uint8_t> expected;
    for (int i = 0; i < 3; i++) {
        std::vector<uint8_t> frame = pattern(500, static_cast<uint8_t>(10 + i));
        Fw::Buffer b(frame.data(), frame.size());
        CHECK(drv.send_handler_public(0, b) == Drv::ByteStreamStatus::OP_OK);
        expected.insert(expected.end(), frame.begin(), frame.end());
    }
    g_uart.transmitAll();
    CHECK(g_uart.txOut == expected);
}

static void test_send_rejects_whole_frame_and_recovers() {
    std::puts("send: frame that does not fit is rejected whole; ready re-signalled after recovery");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    CHECK(drv.h_readyCount == 1);  // configure() signalled ready
    std::vector<uint8_t> big = pattern(Cfg::TX_RING_SIZE - 48, 3);
    Fw::Buffer b1(big.data(), big.size());
    CHECK(drv.send_handler_public(0, b1) == Drv::ByteStreamStatus::OP_OK);
    std::vector<uint8_t> small = pattern(100, 4);
    Fw::Buffer b2(small.data(), small.size());
    CHECK(drv.send_handler_public(0, b2) == Drv::ByteStreamStatus::OTHER_ERROR);
    CHECK(g_uart.txOut.empty());  // nothing partial written
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_txDropEvents.size() == 1);
    CHECK(drv.h_txDropEvents[0].first == 100);
    CHECK(drv.h_txDropEvents[0].second == 1);
    CHECK(drv.h_tlmTxDrops == 1);
    CHECK(drv.h_readyCount == 1);  // ring still full: no recovery yet
    // One interrupt: the device FIFO (32 B) takes a little, far below TX_RESUME_THRESHOLD
    g_uart.pumpIsr();
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_readyCount == 1);
    CHECK(drv.h_txDropEvents.size() == 1);  // same count: no new event
    // Drain everything: recovery signalled once, not again on later ticks
    g_uart.transmitAll();
    CHECK(g_uart.txOut == big);
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_readyCount == 2);
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_readyCount == 2);
    // The caller retried after ready: the frame goes through now
    CHECK(drv.send_handler_public(0, b2) == Drv::ByteStreamStatus::OP_OK);
    g_uart.transmitAll();
    std::vector<uint8_t> expected = big;
    expected.insert(expected.end(), small.begin(), small.end());
    CHECK(g_uart.txOut == expected);
}

static void test_send_oversize_and_degenerate_frames() {
    std::puts("send: frame larger than the ring is always rejected; empty frame is a no-op; no device fails");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    std::vector<uint8_t> huge = pattern(Cfg::TX_RING_SIZE + 1, 5);
    Fw::Buffer b1(huge.data(), huge.size());
    CHECK(drv.send_handler_public(0, b1) == Drv::ByteStreamStatus::OTHER_ERROR);
    Fw::Buffer empty(huge.data(), 0);
    CHECK(drv.send_handler_public(0, empty) == Drv::ByteStreamStatus::OP_OK);
    g_uart.transmitAll();
    CHECK(g_uart.txOut.empty());
    drv.schedIn_handler_public(0, 0);
    CHECK(drv.h_tlmTxDrops == 1);
    CHECK(drv.h_txDropEvents.size() == 1);
    CHECK(drv.h_txDropEvents[0].first == Cfg::TX_RING_SIZE + 1);

    resetAll();
    ZephyrUartDriver unconfigured("uart2");
    std::vector<uint8_t> frame = pattern(10, 6);
    Fw::Buffer b2(frame.data(), frame.size());
    CHECK(unconfigured.send_handler_public(0, b2) == Drv::ByteStreamStatus::OTHER_ERROR);
}

static void test_recv_return_deallocates() {
    std::puts("recvReturnIn: buffer is returned to the allocator");
    resetAll();
    ZephyrUartDriver drv("uart");
    drv.configure(&g_dev, 115200);
    U8 storage[4];
    Fw::Buffer b(storage, sizeof storage);
    drv.recvReturnIn_handler_public(0, b);
    CHECK(drv.h_deallocCount == 1);
}

int main() {
    test_configure_enables_rx_and_signals_ready();
    test_configure_not_ready_device();
    test_full_ring_drained_in_one_tick();
    test_backpressure_no_loss();
    test_alloc_failure_keeps_data();
    test_alloc_failure_while_paused_keeps_backpressure();
    test_overrun_reported_once_per_change();
    test_start_makes_task_sole_consumer();
    test_start_failure_falls_back_to_sched();
    test_start_without_configure_returns_status();
    test_configure_logs_missing_irq_api();
    test_drain_bounded_under_continuous_refill();
    test_send_queues_frame_isr_transmits();
    test_send_multiple_frames_in_order();
    test_send_rejects_whole_frame_and_recovers();
    test_send_oversize_and_degenerate_frames();
    test_recv_return_deallocates();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d check(s) FAILED\n", g_failures);
        return EXIT_FAILURE;
    }
    std::puts("All ZephyrUartDriver tests passed");
    return EXIT_SUCCESS;
}
