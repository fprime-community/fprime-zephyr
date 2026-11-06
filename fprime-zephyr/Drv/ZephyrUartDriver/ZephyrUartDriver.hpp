// ======================================================================
// \title  ZephyrUartDriver.hpp
// \author ethanchee
// \brief  hpp file for ZephyrUartDriver component implementation class
// ======================================================================

#ifndef ZephyrUartDriver_HPP
#define ZephyrUartDriver_HPP

#include "fprime-zephyr/Drv/ZephyrUartDriver/ZephyrUartDriverComponentAc.hpp"
#include "zephyr-config/ZephyrUartDriverCfg.hpp"

#include <Os/Task.hpp>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <atomic>

namespace Zephyr {

  class ZephyrUartDriver :
    public ZephyrUartDriverComponentBase
  {

    const FwSizeType SERIAL_BUFFER_SIZE = 64;

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

        void configure(const struct device *dev, U32 baud_rate,
                       enum uart_config_flow_control flowControl = UART_CFG_FLOW_CTRL_NONE);

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

        const struct device *m_dev;

    std::atomic<U32> m_rxAllocFails;  //!< Drain stalls for lack of an Fw::Buffer (drain writes, schedIn reads)
    U32 m_rxOverrunsReported;         //!< Overruns already reported via event (schedIn context only)

    struct ring_buf m_txRing;                            //!< TX ring (producer: send, consumer: ISR)
    U8 m_txRingData[ZephyrUartDriverCfg::TX_RING_SIZE];  //!< TX ring storage

    std::atomic<bool> m_txStalled;      //!< A frame was rejected; `ready` owed once the ring recovers
    std::atomic<U32> m_txBytes;         //!< Bytes moved from the TX ring into the device
    std::atomic<U32> m_txDrops;         //!< Frames rejected by send (send writes, schedIn reads)
    std::atomic<U32> m_txLastDropSize;  //!< Size of the most recently rejected frame
    U32 m_txDropsReported;              //!< Drops already reported via event (schedIn context only)
};

}  // end namespace Zephyr

#endif
