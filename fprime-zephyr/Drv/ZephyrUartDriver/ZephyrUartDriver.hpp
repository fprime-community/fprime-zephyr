// ======================================================================
// \title  ZephyrUartDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrUartDriver component implementation class
// ======================================================================

#ifndef ZephyrUartDriver_HPP
#define ZephyrUartDriver_HPP

#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriverComponentAc.hpp"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#define RING_BUF_SIZE 1024

enum class ActiveUart {
    BLE,
    USB
};

namespace Zephyr {

  class ZephyrUartDriver : public ZephyrUartDriverComponentBase {

    static constexpr FwSizeType SERIAL_BUFFER_SIZE = 64;

    public:

        // ----------------------------------------------------------------------
        // Construction, initialization, and destruction
        // ----------------------------------------------------------------------

        //! Construct object ZephyrUartDriver
        //!
        ZephyrUartDriver(
            const char *const compName /*!< The component name*/
        );

        //! Destroy object ZephyrUartDriver
        //!
        ~ZephyrUartDriver();

        void configure(const struct device *bleDev, const struct device *usbDev, U32 baud_rate);

    public:

        static void serial_cb(const struct device *dev, void *user_data);

        // ----------------------------------------------------------------------
        // Handler implementations for user-defined typed input ports
        // ----------------------------------------------------------------------

        //! Handler implementation for schedIn
        //!
        void schedIn_handler(
            const FwIndexType portNum, /*!< The port number*/
            U32 context /*!< 
        The call order
        */
        );


        //! Handler implementation for send
        //!
        Drv::ByteStreamStatus send_handler(
            const FwIndexType portNum, /*!< The port number*/
            Fw::Buffer &sendBuffer 
        );

        void recvReturnIn_handler(
            const FwIndexType portNum, /*!< The port number*/
            Fw::Buffer &returnBuffer
        );

        /* separate ring buffers for BLE and USB because each link produces bytes independently on its own interrupt. Keeping one shared ring buffer
           for both makes the drain policy messier and it's harder to determine which link was the last to receive data (used to determine which link to send data to)
        */
        const struct device *m_ble_dev;
        U8 m_ring_buf_ble_data[RING_BUF_SIZE];
        struct ring_buf m_ring_buf_ble;
        bool m_rx_throttled_ble;

        const struct device *m_usb_dev;
        U8 m_ring_buf_usb_data[RING_BUF_SIZE];
        struct ring_buf m_ring_buf_usb;
        bool m_rx_throttled_usb; 

        /* used to determine which link received data last and therefore, which link to send data to */
        U32 m_last_rx_ble_ms;
        U32 m_last_rx_usb_ms;
    };

} // end namespace Zephyr

#endif
