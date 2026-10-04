// Mock Zephyr interrupt-driven UART API backed by host-side FIFOs
#ifndef ZEPHYR_DRIVERS_UART_H
#define ZEPHYR_DRIVERS_UART_H
#include <stdint.h>
#include <zephyr/device.h>
#ifdef __cplusplus
extern "C" {
#endif
enum uart_config_parity { UART_CFG_PARITY_NONE };
enum uart_config_stop_bits { UART_CFG_STOP_BITS_1 };
enum uart_config_data_bits { UART_CFG_DATA_BITS_8 };
enum uart_config_flow_control { UART_CFG_FLOW_CTRL_NONE };
enum uart_rx_stop_reason { UART_ERROR_OVERRUN = (1 << 0), UART_ERROR_PARITY = (1 << 1), UART_ERROR_FRAMING = (1 << 2) };
struct uart_config {
    uint32_t baudrate;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t data_bits;
    uint8_t flow_ctrl;
};
typedef void (*uart_irq_callback_user_data_t)(const struct device* dev, void* user_data);
int uart_configure(const struct device* dev, const struct uart_config* cfg);
int uart_irq_callback_user_data_set(const struct device* dev, uart_irq_callback_user_data_t cb, void* user_data);
void uart_irq_rx_enable(const struct device* dev);
void uart_irq_rx_disable(const struct device* dev);
void uart_irq_tx_enable(const struct device* dev);
void uart_irq_tx_disable(const struct device* dev);
int uart_irq_update(const struct device* dev);
int uart_irq_rx_ready(const struct device* dev);
int uart_irq_tx_ready(const struct device* dev);
int uart_fifo_read(const struct device* dev, uint8_t* rx_data, const int size);
int uart_fifo_fill(const struct device* dev, const uint8_t* tx_data, int size);
int uart_err_check(const struct device* dev);
void uart_poll_out(const struct device* dev, unsigned char out_char);
#ifdef __cplusplus
}
#endif
#endif
