// ======================================================================
// \title  ZephyrUartDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrUartDriver component implementation class
// ======================================================================


#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Types/Assert.hpp"
#include <Fw/FPrimeBasicTypes.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ZephyrUartDriver, LOG_LEVEL_INF);

namespace Zephyr {

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    ZephyrUartDriver ::
        ZephyrUartDriver(const char *const compName) : ZephyrUartDriverComponentBase(compName), 
        m_rx_throttled_ble(false),
        m_rx_throttled_usb(false),
        m_last_rx_ble_ms(0),
        m_last_rx_usb_ms(0)
    {
    }

    ZephyrUartDriver ::
        ~ZephyrUartDriver()
    {

    }

    void ZephyrUartDriver::configure(const struct device *bleDev, const struct device *usbDev, U32 baud_rate) {
        FW_ASSERT(bleDev != nullptr);
        FW_ASSERT(usbDev != nullptr);
        m_ble_dev = bleDev;
        m_usb_dev = usbDev;

        bool ble_ready = device_is_ready(this->m_ble_dev);
        bool usb_ready = device_is_ready(this->m_usb_dev);

        if (!ble_ready && !usb_ready) {
            LOG_ERR("UART devices not ready");
            return;
        }

        struct uart_config uart_cfg = {
            .baudrate = baud_rate,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = UART_CFG_STOP_BITS_1,
            .data_bits = UART_CFG_DATA_BITS_8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
        };

        if (ble_ready) {
            uart_configure(this->m_ble_dev, &uart_cfg);
            ring_buf_init(&this->m_ring_buf_ble, RING_BUF_SIZE, this->m_ring_buf_ble_data);
            uart_irq_callback_user_data_set(this->m_ble_dev, serial_cb, this);
            uart_irq_rx_enable(this->m_ble_dev);
            uart_irq_tx_disable(this->m_ble_dev);
        }
        if (usb_ready) {
            uart_configure(this->m_usb_dev, &uart_cfg);
            ring_buf_init(&this->m_ring_buf_usb, RING_BUF_SIZE, this->m_ring_buf_usb_data);
            uart_irq_callback_user_data_set(this->m_usb_dev, serial_cb, this);
            uart_irq_rx_enable(this->m_usb_dev);
            uart_irq_tx_disable(this->m_usb_dev);
        }

        if (this->isConnected_ready_OutputPort(0)) {
            this->ready_out(0);
        }
    }

    void ZephyrUartDriver::serial_cb(const struct device *dev, void *user_data)
    {
        struct ZephyrUartDriver *self = reinterpret_cast<ZephyrUartDriver *>(user_data);

        while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
            if (dev == self->m_ble_dev) {
                if (!self->m_rx_throttled_ble && uart_irq_rx_ready(dev)) {
                    int recv_len, rb_len;
                    uint8_t buffer[SERIAL_BUFFER_SIZE];

                    // len = amount of space in the ring buffer
                    size_t len = MIN(ring_buf_space_get(&self->m_ring_buf_ble), sizeof(buffer));
        
                    if (len == 0) {
                        /* Throttle because ring buffer is full */
                        uart_irq_rx_disable(dev);
                        self->m_rx_throttled_ble = true;
                        continue;
                    }
        
                    // recv_len = amount of bytes read from the UART FIFO
                    recv_len = uart_fifo_read(dev, buffer, len);
                    if (recv_len < 0) {
                        LOG_ERR("Failed to read UART FIFO");
                        recv_len = 0;
                    };

                    // rb_len = amount of bytes written to the ring buffer
                    rb_len = ring_buf_put(&self->m_ring_buf_ble, buffer, recv_len);
                    if (rb_len < recv_len) {
                        LOG_ERR("Drop %u bytes", recv_len - rb_len);
                    }
        
                    LOG_INF("IRQ rx: %d bytes -> ringbuf", rb_len);

                    // update the last RX time for the BLE link
                    if (rb_len > 0) {
                        self->m_last_rx_ble_ms = k_uptime_get_32();
                    }
                }
                
            } else if (dev == self->m_usb_dev) {

                if (!self->m_rx_throttled_usb && uart_irq_rx_ready(dev)) {
                    int recv_len, rb_len;
                    uint8_t buffer[SERIAL_BUFFER_SIZE];
                    size_t len = MIN(ring_buf_space_get(&self->m_ring_buf_usb),
                            sizeof(buffer));
        
                    if (len == 0) {
                        /* Throttle because ring buffer is full */
                        uart_irq_rx_disable(dev);
                        self->m_rx_throttled_usb = true;
                        continue;
                    }
        
                    recv_len = uart_fifo_read(dev, buffer, len);
                    if (recv_len < 0) {
                        LOG_ERR("Failed to read UART FIFO");
                        recv_len = 0;
                    };
        
                    rb_len = ring_buf_put(&self->m_ring_buf_usb, buffer, recv_len);
                    if (rb_len < recv_len) {
                        LOG_ERR("Drop %u bytes", recv_len - rb_len);
                    }
                    if (rb_len > 0) {
                        self->m_last_rx_usb_ms = k_uptime_get_32();
                    }
        
                    LOG_INF("IRQ rx: %d bytes -> ringbuf", rb_len);
                }
            }
        }
    
    }

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    void ZephyrUartDriver :: schedIn_handler(const FwIndexType portNum, U32 context) {

        Fw::Buffer recv_buffer = this->allocate_out(0, SERIAL_BUFFER_SIZE);

        ActiveUart active_uart = this->m_last_rx_ble_ms >= this->m_last_rx_usb_ms ? ActiveUart::BLE : ActiveUart::USB;
        if (active_uart == ActiveUart::BLE) {

            U32 recv_size = ring_buf_get(&this->m_ring_buf_ble, recv_buffer.getData(), recv_buffer.getSize());
            if (recv_size == 0) {
                // no data received, deallocate buffer
                this->deallocate_out(0, recv_buffer);
            } else {
                recv_buffer.setSize(recv_size);
                this->recv_out(0, recv_buffer, Drv::ByteStreamStatus::OP_OK);
            }

            if (this->m_rx_throttled_ble) {
                uart_irq_rx_enable(this->m_ble_dev);
                this->m_rx_throttled_ble = false;
            }

        } else {
            
            U32 recv_size = ring_buf_get(&this->m_ring_buf_usb, recv_buffer.getData(), recv_buffer.getSize());
            if (recv_size == 0) {
                // no data received, deallocate buffer
                this->deallocate_out(0, recv_buffer);
            } else {
                recv_buffer.setSize(recv_size);
                this->recv_out(0, recv_buffer, Drv::ByteStreamStatus::OP_OK);
            }

            if (this->m_rx_throttled_usb) {
                uart_irq_rx_enable(this->m_usb_dev);
                this->m_rx_throttled_usb = false;
            }
        }
    }

    Drv::ByteStreamStatus ZephyrUartDriver :: send_handler(const FwIndexType portNum, Fw::Buffer &sendBuffer) {
        ActiveUart active_uart = this->m_last_rx_ble_ms >= this->m_last_rx_usb_ms ? ActiveUart::BLE : ActiveUart::USB;
        if (active_uart == ActiveUart::BLE) {
            for (U32 i = 0; i < sendBuffer.getSize(); i++) {
                uart_poll_out(this->m_ble_dev, sendBuffer.getData()[i]);
            }
        } else {
            for (U32 i = 0; i < sendBuffer.getSize(); i++) {
                uart_poll_out(this->m_usb_dev, sendBuffer.getData()[i]);
            }
        }
        return Drv::ByteStreamStatus::OP_OK;
    }

    void ZephyrUartDriver ::recvReturnIn_handler(const FwIndexType portNum, Fw::Buffer &returnBuffer) {
        this->deallocate_out(0, returnBuffer);
    }

} // end namespace Zephyr
