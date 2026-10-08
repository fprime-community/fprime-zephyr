// Host-side implementations of the mocked Zephyr APIs used by ZephyrUartDriver
#include "zephyr_mocks.hpp"

#include <algorithm>
#include <cstring>

MockUart g_uart;
MockSem g_sem;

// --- kernel.h ---
extern "C" int k_sem_init(struct k_sem* sem, unsigned int initial, unsigned int limit) {
    sem->count = initial;
    sem->limit = limit;
    g_sem.initCount++;
    return 0;
}
extern "C" void k_sem_give(struct k_sem* sem) {
    g_sem.giveCount++;
    if (sem->count < sem->limit) {
        sem->count++;
    }
}
extern "C" int k_sem_take(struct k_sem* sem, k_timeout_t) {
    g_sem.takeCount++;
    if (g_sem.onTake != nullptr) {
        g_sem.onTake(g_sem.takeCount);
    }
    if (sem->count > 0) {
        sem->count--;
        return 0;
    }
    return -EAGAIN;
}

// --- device.h ---
extern "C" bool device_is_ready(const struct device*) {
    return g_uart.deviceReady;
}

// --- drivers/uart.h ---
extern "C" int uart_configure(const struct device*, const struct uart_config* cfg) {
    g_uart.configuredBaud = cfg->baudrate;
    return 0;
}
extern "C" int uart_irq_callback_user_data_set(const struct device* dev,
                                               uart_irq_callback_user_data_t cb,
                                               void* user_data) {
    if (g_uart.callbackSetResult != 0) {
        return g_uart.callbackSetResult;
    }
    g_uart.dev = dev;
    g_uart.callback = cb;
    g_uart.userData = user_data;
    return 0;
}
extern "C" void uart_irq_rx_enable(const struct device*) {
    g_uart.rxIrqEnabled = true;
    g_uart.rxEnableCount++;
}
extern "C" void uart_irq_rx_disable(const struct device*) {
    g_uart.rxIrqEnabled = false;
    g_uart.rxDisableCount++;
}
extern "C" void uart_irq_tx_enable(const struct device*) {
    g_uart.txIrqEnabled = true;
}
extern "C" void uart_irq_tx_disable(const struct device*) {
    g_uart.txIrqEnabled = false;
}
extern "C" int uart_irq_update(const struct device*) {
    return 1;
}
extern "C" int uart_irq_rx_ready(const struct device*) {
    return (g_uart.rxIrqEnabled && !g_uart.rxFifo.empty()) ? 1 : 0;
}
extern "C" int uart_irq_tx_ready(const struct device*) {
    return (g_uart.txIrqEnabled && (g_uart.txFifoFree() > 0)) ? 1 : 0;
}
extern "C" int uart_fifo_read(const struct device*, uint8_t* rx_data, const int size) {
    const size_t n = std::min(static_cast<size_t>(size), std::min(g_uart.rxFifo.size(), g_uart.rxReadLimit));
    std::memcpy(rx_data, g_uart.rxFifo.data(), n);
    g_uart.rxFifo.erase(g_uart.rxFifo.begin(), g_uart.rxFifo.begin() + static_cast<long>(n));
    return static_cast<int>(n);
}
extern "C" int uart_fifo_fill(const struct device*, const uint8_t* tx_data, int size) {
    const size_t n = std::min(static_cast<size_t>(size), g_uart.txFifoFree());
    g_uart.txOut.insert(g_uart.txOut.end(), tx_data, tx_data + n);
    g_uart.txPending += n;
    return static_cast<int>(n);
}
extern "C" int uart_err_check(const struct device*) {
    const int err = g_uart.pendingErrors;
    g_uart.pendingErrors = 0;
    return err;
}
extern "C" void uart_poll_out(const struct device*, unsigned char out_char) {
    g_uart.txOut.push_back(static_cast<uint8_t>(out_char));
}

// --- sys/ring_buffer.h: mirrors the Zephyr semantics (contiguous claims, finish <= claim) ---
extern "C" void ring_buf_init(struct ring_buf* buf, uint32_t size, uint8_t* data) {
    buf->buffer = data;
    buf->size = size;
    buf->head = buf->tail = buf->count = 0;
    buf->put_claimed = buf->get_claimed = 0;
}
extern "C" uint32_t ring_buf_capacity_get(struct ring_buf* buf) {
    return buf->size;
}
extern "C" uint32_t ring_buf_size_get(struct ring_buf* buf) {
    return buf->count;
}
extern "C" uint32_t ring_buf_space_get(struct ring_buf* buf) {
    return buf->size - buf->count;
}
extern "C" int ring_buf_is_empty(struct ring_buf* buf) {
    return buf->count == 0;
}
extern "C" uint32_t ring_buf_put_claim(struct ring_buf* buf, uint8_t** data, uint32_t size) {
    const uint32_t write = (buf->tail + buf->put_claimed) % buf->size;
    const uint32_t free = buf->size - buf->count - buf->put_claimed;
    const uint32_t contiguous = buf->size - write;
    const uint32_t n = std::min(size, std::min(free, contiguous));
    *data = buf->buffer + write;
    buf->put_claimed += n;
    return n;
}
extern "C" int ring_buf_put_finish(struct ring_buf* buf, uint32_t size) {
    if (size > buf->put_claimed) {
        return -EINVAL;
    }
    buf->tail = (buf->tail + size) % buf->size;
    buf->count += size;
    buf->put_claimed = 0;
    return 0;
}
extern "C" uint32_t ring_buf_put(struct ring_buf* buf, const uint8_t* data, uint32_t size) {
    uint32_t total = 0;
    while (total < size) {
        uint8_t* dst = nullptr;
        const uint32_t n = ring_buf_put_claim(buf, &dst, size - total);
        if (n == 0) {
            break;
        }
        std::memcpy(dst, data + total, n);
        total += n;
        ring_buf_put_finish(buf, n);
    }
    return total;
}
extern "C" uint32_t ring_buf_get_claim(struct ring_buf* buf, uint8_t** data, uint32_t size) {
    const uint32_t read = (buf->head + buf->get_claimed) % buf->size;
    const uint32_t avail = buf->count - buf->get_claimed;
    const uint32_t contiguous = buf->size - read;
    const uint32_t n = std::min(size, std::min(avail, contiguous));
    *data = buf->buffer + read;
    buf->get_claimed += n;
    return n;
}
extern "C" int ring_buf_get_finish(struct ring_buf* buf, uint32_t size) {
    if (size > buf->get_claimed) {
        return -EINVAL;
    }
    buf->head = (buf->head + size) % buf->size;
    buf->count -= size;
    buf->get_claimed = 0;
    return 0;
}
extern "C" uint32_t ring_buf_get(struct ring_buf* buf, uint8_t* data, uint32_t size) {
    uint32_t total = 0;
    while (total < size) {
        uint8_t* src = nullptr;
        const uint32_t n = ring_buf_get_claim(buf, &src, size - total);
        if (n == 0) {
            break;
        }
        std::memcpy(data + total, src, n);
        total += n;
        ring_buf_get_finish(buf, n);
    }
    return total;
}
