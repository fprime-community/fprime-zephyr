// ======================================================================
// \title  ZephyrUartDriver.cpp
// \author ethanchee
// \brief  cpp file for ZephyrUartDriver component implementation class
// ======================================================================


#include <Zephyr/Drv/ZephyrUartDriver/ZephyrUartDriver.hpp>
#include "Fw/Types/BasicTypes.hpp"
#include "Fw/Types/Assert.hpp"
#include <FpConfig.hpp>

namespace Zephyr {

    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    ZephyrUartDriver ::
        ZephyrUartDriver(
            const char *const compName
        ) : ZephyrUartDriverComponentBase(compName)
    {

    }

    ZephyrUartDriver ::
        ~ZephyrUartDriver()
    {

    }

    void ZephyrUartDriver::configure(const struct device *dev, U32 baud_rate) {
        FW_ASSERT(dev != nullptr);
        m_dev = dev;

        if (!device_is_ready(this->m_dev)) {
            return;
        }

        struct uart_config uart_cfg = {
            .baudrate = baud_rate,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = UART_CFG_STOP_BITS_1,
            .data_bits = UART_CFG_DATA_BITS_8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
        };
        uart_configure(this->m_dev, &uart_cfg);

        ring_buf_init(&this->m_ring_buf, RING_BUF_SIZE, this->m_ring_buf_data);
        uart_irq_callback_user_data_set(this->m_dev, serial_cb, &this->m_ring_buf);

        uart_irq_rx_enable(this->m_dev);
	    uart_irq_tx_disable(this->m_dev);

        if (this->isConnected_ready_OutputPort(0)) {
            this->ready_out(0);
        }
    }

    void ZephyrUartDriver::serial_cb(const struct device *dev, void *user_data)
    {
        struct ring_buf *ring_buf = reinterpret_cast<struct ring_buf *>(user_data);

        if (!uart_irq_update(dev)) {
            return;
        }

        if (!uart_irq_rx_ready(dev)) {
            return;
        }

        U8 c;
        // TODO: Get rid of the endless loop (in an IRQ handler!).
        while (uart_fifo_read(dev, &c, 1) == 1) {
            if (ring_buf_put(ring_buf, &c, 1) != 1) {
                // TODO: Handle properly.
                printk("UART buffer overrun\n");
            }
        }
    }

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    void ZephyrUartDriver ::
        schedIn_handler(
            const NATIVE_INT_TYPE portNum,
            NATIVE_UINT_TYPE context
        )
    {
        Fw::Buffer recv_buffer = this->allocate_out(0, SERIAL_BUFFER_SIZE);

        U32 recv_size = ring_buf_get(&this->m_ring_buf, recv_buffer.getData(), recv_buffer.getSize());
        recv_buffer.setSize(recv_size);

        recv_out(0, recv_buffer, Drv::RecvStatus::RECV_OK);
    }

    Drv::SendStatus ZephyrUartDriver ::
        send_handler(
            const NATIVE_INT_TYPE portNum,
            Fw::Buffer &sendBuffer
        )
    {
        for (U32 i = 0; i < sendBuffer.getSize(); i++) {
            uart_poll_out(this->m_dev, sendBuffer.getData()[i]);
        }

        if (this->isConnected_deallocate_OutputPort(0)) {
            deallocate_out(0, sendBuffer);
        }
        return Drv::SendStatus::SEND_OK;
    }

} // end namespace Zephyr
