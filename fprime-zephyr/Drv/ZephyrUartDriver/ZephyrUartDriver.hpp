// ======================================================================
// \title  ZephyrUartDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrUartDriver component implementation class
// ======================================================================

#ifndef ZephyrUartDriver_HPP
#define ZephyrUartDriver_HPP

#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriverComponentAc.hpp"

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#define RING_BUF_SIZE 1024

namespace Zephyr {

class ZephyrUartDriver : public ZephyrUartDriverComponentBase {
    const FwSizeType SERIAL_BUFFER_SIZE = 64;

  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object ZephyrUartDriver
    //!
    ZephyrUartDriver(const char* const compName /*!< The component name*/
    );

    //! Destroy object ZephyrUartDriver
    //!
    ~ZephyrUartDriver();

    void configure(const struct device* dev, /*!< The Zephyr device structure for the UART */
                   U32 baud_rate,            /*!< The desired baud rate (e.g., 115200) */
                   uart_config_flow_control flow_ctrl =
                       UART_CFG_FLOW_CTRL_NONE /*!< Hardware flow control setting (default: None). Use
                                                  UART_CFG_FLOW_CTRL_RTS_CTS to enable RTS/CTS. */
    );

  public:
    static void serial_cb(const struct device* dev, void* user_data);

    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for schedIn
    //!
    void schedIn_handler(const FwIndexType portNum, /*!< The port number*/
                         U32 context                /*!<
                                    The call order
                                    */
    );

    //! Handler implementation for send
    //!
    Drv::ByteStreamStatus send_handler(const FwIndexType portNum, /*!< The port number*/
                                       Fw::Buffer& sendBuffer);

    void recvReturnIn_handler(const FwIndexType portNum, /*!< The port number*/
                              Fw::Buffer& returnBuffer);

    const struct device* m_dev;

    U8 m_ring_buf_data[RING_BUF_SIZE];
    struct ring_buf m_ring_buf;
};

}  // end namespace Zephyr

#endif
