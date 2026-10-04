// Test-visible state of the mocked Zephyr UART, semaphore and ring buffer
#ifndef ZEPHYR_MOCKS_HPP
#define ZEPHYR_MOCKS_HPP
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <vector>

struct MockUart {
    bool deviceReady = true;
    uint32_t configuredBaud = 0;
    const struct device* dev = nullptr;
    uart_irq_callback_user_data_t callback = nullptr;
    void* userData = nullptr;
    bool rxIrqEnabled = false;
    bool txIrqEnabled = false;
    int rxEnableCount = 0;
    int rxDisableCount = 0;
    int pendingErrors = 0;
    std::vector<uint8_t> rxFifo;                 // bytes the "device" has for us
    size_t rxReadLimit = static_cast<size_t>(-1); // max bytes per uart_fifo_read (device FIFO depth)
    std::vector<uint8_t> txOut;                  // bytes written to the "device"
    size_t txFifoDepth = 32;
    size_t txPending = 0;
    size_t txFifoFree() const { return txFifoDepth > txPending ? txFifoDepth - txPending : 0; }

    void reset() { *this = MockUart(); }
    // Deliver interrupts while the device has data and the RX interrupt is enabled
    void pumpIsr() {
        for (int guard = 0; (guard < 100000) && (callback != nullptr) && (uart_irq_rx_ready(nullptr) || uart_irq_tx_ready(nullptr)); guard++) {
            callback(dev, userData);
        }
    }
};

struct MockSem {
    int initCount = 0;
    int giveCount = 0;
    int takeCount = 0;
    void (*onTake)(int takeCount) = nullptr;
    void reset() { *this = MockSem(); }
};

extern MockUart g_uart;
extern MockSem g_sem;
#endif
