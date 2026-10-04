// ======================================================================
// \title  ZephyrUartDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrUartDriver component implementation class
// ======================================================================

#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.hpp"

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/Assert.hpp>

namespace Zephyr {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

ZephyrUartDriver ::ZephyrUartDriver(const char* const compName)
    : ZephyrUartDriverComponentBase(compName),
      m_dev(nullptr),
      m_taskStarted(false),
      m_quit(false),
      m_rxPaused(false),
      m_rxBytes(0),
      m_rxOverruns(0),
      m_rxPauseCount(0),
      m_rxAllocFails(0),
      m_rxOverrunsReported(0) {
    ring_buf_init(&this->m_rxRing, static_cast<uint32_t>(ZephyrUartDriverCfg::RX_RING_SIZE), this->m_rxRingData);
    (void)k_sem_init(&this->m_wakeSem, 0, 1);
}

ZephyrUartDriver ::~ZephyrUartDriver() {}

void ZephyrUartDriver ::configure(const struct device* dev, U32 baud_rate) {
    FW_ASSERT(dev != nullptr);
    FW_ASSERT(this->m_dev == nullptr);  // configure once

    if (!device_is_ready(dev)) {
        Fw::Logger::log("ZephyrUartDriver: device %s is not ready\n", dev->name);
        return;
    }
    this->m_dev = dev;

    struct uart_config uart_cfg = {
        .baudrate = baud_rate,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
    };
    // Not supported (-ENOSYS/-ENOTSUP) by every device, e.g. CDC ACM ignores the baud rate
    (void)uart_configure(this->m_dev, &uart_cfg);

    // -ENOSYS/-ENOTSUP: device lacks the interrupt-driven API, so receive would never deliver
    const int cbStatus = uart_irq_callback_user_data_set(this->m_dev, serial_cb, this);
    if (cbStatus != 0) {
        Fw::Logger::log("ZephyrUartDriver: %s has no interrupt-driven UART API (%d), receive disabled\n", dev->name,
                        cbStatus);
    }
    uart_irq_tx_disable(this->m_dev);
    uart_irq_rx_enable(this->m_dev);

    if (this->isConnected_ready_OutputPort(0)) {
        this->ready_out(0);
    }
}

Os::Task::Status ZephyrUartDriver ::start(FwTaskPriorityType priority, FwSizeType stackSize) {
    FW_ASSERT(!this->m_taskStarted);  // start at most once
    if (this->m_dev == nullptr) {
        // configure() was skipped or found the device not ready: nothing to receive, so no task
        Fw::Logger::log("ZephyrUartDriver: start() without a configured device, draining from schedIn\n");
        return Os::Task::INVALID_HANDLE;
    }

    // Claim the ring before the task exists so schedIn stops draining before the task begins
    this->m_taskStarted = true;
    Os::TaskString name("UartRx");
    Os::Task::Arguments arguments(name, rxTaskEntry, this, priority, stackSize);
    const Os::Task::Status status = this->m_rxTask.start(arguments);
    if (status != Os::Task::OP_OK) {
        // Degrade to the rate-group drain rather than asserting at boot
        this->m_taskStarted = false;
        Fw::Logger::log("ZephyrUartDriver: RX task start failed (%d), draining from schedIn\n",
                        static_cast<int>(status));
    }
    return status;
}

void ZephyrUartDriver ::stop() {
    this->m_quit = true;
    k_sem_give(&this->m_wakeSem);
}

Os::Task::Status ZephyrUartDriver ::join() {
    if (!this->m_taskStarted) {
        return Os::Task::OP_OK;
    }
    return this->m_rxTask.join();
}

// ----------------------------------------------------------------------
// Interrupt-side processing
// ----------------------------------------------------------------------

void ZephyrUartDriver ::serial_cb(const struct device* dev, void* user_data) {
    ZephyrUartDriver* self = static_cast<ZephyrUartDriver*>(user_data);
    if ((self == nullptr) || (dev == nullptr) || (self->m_dev != dev)) {
        return;
    }
    if (uart_irq_update(dev) != 1) {
        return;
    }

    bool received = false;
    if (uart_irq_rx_ready(dev) > 0) {
        received = self->isrReceive();
    }
    if (received && self->m_taskStarted) {
        k_sem_give(&self->m_wakeSem);
    }
}

bool ZephyrUartDriver ::isrReceive() {
    // Hardware overrun flags (device FIFO overflowed). CDC ACM and some drivers do not implement
    // err_check and return a negative errno, which is ignored.
    const int errors = uart_err_check(this->m_dev);
    if ((errors > 0) && ((static_cast<unsigned int>(errors) & UART_ERROR_OVERRUN) != 0)) {
        this->m_rxOverruns++;
    }

    bool received = false;
    // Read directly into the ring: a claim covers one contiguous free region, so a full ring
    // takes at most two claims and the third returns 0
    for (FwSizeType i = 0; i < MAX_ISR_CLAIMS; i++) {
        U8* dst = nullptr;
        const FwSizeType claimed =
            ring_buf_put_claim(&this->m_rxRing, &dst, static_cast<uint32_t>(ZephyrUartDriverCfg::RX_RING_SIZE));
        if (claimed == 0) {
            // Ring full: stop reading so the device (or the USB host) holds the data instead
            uart_irq_rx_disable(this->m_dev);
            this->m_rxPaused = true;
            this->m_rxPauseCount++;
            break;
        }
        const int read = uart_fifo_read(this->m_dev, dst, static_cast<int>(claimed));
        if (read <= 0) {
            (void)ring_buf_put_finish(&this->m_rxRing, 0);
            break;
        }
        if (ring_buf_put_finish(&this->m_rxRing, static_cast<uint32_t>(read)) != 0) {
            // Cannot happen (read <= claimed); treat as lost data rather than trusting the ring
            this->m_rxOverruns++;
            break;
        }
        this->m_rxBytes += static_cast<U32>(read);
        received = true;
        if (static_cast<FwSizeType>(read) < claimed) {
            break;  // device FIFO drained
        }
    }
    return received;
}

// ----------------------------------------------------------------------
// Task-side processing
// ----------------------------------------------------------------------

void ZephyrUartDriver ::drainRx() {
    for (FwSizeType i = 0; i < MAX_DRAIN_ITERATIONS; i++) {
        const FwSizeType available = ring_buf_size_get(&this->m_rxRing);
        if (available == 0) {
            break;
        }
        const FwSizeType request = FW_MIN(available, ZephyrUartDriverCfg::RX_CHUNK_SIZE);
        Fw::Buffer buffer = this->allocate_out(0, request);
        if ((buffer.getData() == nullptr) || (buffer.getSize() == 0)) {
            // Allocator exhausted (e.g. Svc::BufferManager returns an empty buffer): leave the
            // data in the ring, do not hand the empty buffer back, retry on the next drain
            this->m_rxAllocFails++;
            break;
        }
        const FwSizeType size = FW_MIN(buffer.getSize(), request);
        const FwSizeType got = ring_buf_get(&this->m_rxRing, buffer.getData(), static_cast<uint32_t>(size));
        if (got == 0) {
            this->deallocate_out(0, buffer);
            break;
        }
        buffer.setSize(got);
        // Give the interrupt room to refill while the frame is processed downstream
        this->resumeRxIfPaused();
        this->recv_out(0, buffer, Drv::ByteStreamStatus::OP_OK);
    }
    this->resumeRxIfPaused();
}

void ZephyrUartDriver ::resumeRxIfPaused() {
    const FwSizeType space = ring_buf_space_get(&this->m_rxRing);
    if (this->m_rxPaused && (space >= ZephyrUartDriverCfg::RX_CHUNK_SIZE)) {
        // The RX interrupt is disabled while paused, so the ISR cannot race this clear
        this->m_rxPaused = false;
        uart_irq_rx_enable(this->m_dev);
    }
}

void ZephyrUartDriver ::reportStatus() {
    const U32 overruns = this->m_rxOverruns;
    if (overruns != this->m_rxOverrunsReported) {
        this->m_rxOverrunsReported = overruns;
        this->log_WARNING_LO_RxOverrun(overruns);
    }
    this->tlmWrite_RxBytes(this->m_rxBytes);
    this->tlmWrite_RxOverrunCount(overruns);
    this->tlmWrite_RxBackpressureCount(this->m_rxPauseCount);
    this->tlmWrite_RxAllocFailCount(this->m_rxAllocFails);
}

void ZephyrUartDriver ::rxTaskEntry(void* ptr) {
    FW_ASSERT(ptr != nullptr);
    ZephyrUartDriver* self = static_cast<ZephyrUartDriver*>(ptr);
    while (!self->m_quit) {
        (void)k_sem_take(&self->m_wakeSem, K_MSEC(ZephyrUartDriverCfg::RX_TASK_WAKE_TIMEOUT_MS));
        if (self->m_quit) {
            break;
        }
        self->drainRx();
    }
}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void ZephyrUartDriver ::schedIn_handler(FwIndexType portNum, U32 context) {
    if (this->m_dev == nullptr) {
        return;
    }
    if (this->m_taskStarted) {
        // The RX task is the only reader of the RX ring; just make sure it is awake
        k_sem_give(&this->m_wakeSem);
    } else {
        this->drainRx();
    }
    this->reportStatus();
}

Drv::ByteStreamStatus ZephyrUartDriver ::send_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) {
    if ((this->m_dev == nullptr) || (sendBuffer.getData() == nullptr)) {
        return Drv::ByteStreamStatus::OTHER_ERROR;
    }
    for (FwSizeType i = 0; i < sendBuffer.getSize(); i++) {
        uart_poll_out(this->m_dev, sendBuffer.getData()[i]);
    }
    return Drv::ByteStreamStatus::OP_OK;
}

void ZephyrUartDriver ::recvReturnIn_handler(FwIndexType portNum, Fw::Buffer& returnBuffer) {
    this->deallocate_out(0, returnBuffer);
}

}  // end namespace Zephyr
