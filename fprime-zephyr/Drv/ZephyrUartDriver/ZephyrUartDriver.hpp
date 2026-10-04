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

//! \brief Byte-stream driver for a Zephyr UART (hardware UART or USB CDC ACM)
//!
//! RX: the UART interrupt callback (`serial_cb`) moves bytes from the device FIFO into a
//! software ring buffer. When the ring is full the RX interrupt is disabled (back-pressure)
//! rather than discarding bytes; it is re-enabled by the drain once space is available. The
//! ring is drained in task context by exactly one consumer: the `schedIn` rate-group call
//! (default) or, after `start()` is called from the topology, a dedicated task woken by the
//! interrupt. Once `start()` succeeds, `schedIn` never touches the RX ring again.
//!
//! TX: `send` writes the frame synchronously with `uart_poll_out` on the caller's thread.
//!
//! All storage is in-class; there is no dynamic allocation after `start()`.
class ZephyrUartDriver final : public ZephyrUartDriverComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object ZephyrUartDriver
    explicit ZephyrUartDriver(const char* const compName  //!< The component name
    );

    //! Destroy object ZephyrUartDriver
    ~ZephyrUartDriver() override;

    ZephyrUartDriver(const ZephyrUartDriver&) = delete;
    ZephyrUartDriver& operator=(const ZephyrUartDriver&) = delete;

    //! \brief Configure the UART device and enable receive
    //!
    //! Must be called once from the topology before the rate group runs and before `start()`.
    //! \param dev: Zephyr UART device (hardware UART or CDC ACM instance)
    //! \param baud_rate: baud rate in bits per second (ignored by CDC ACM)
    void configure(const struct device* dev, U32 baud_rate);

    //! \brief Start the optional event-driven RX task
    //!
    //! Creates an Os::Task that becomes the sole consumer of the RX ring, woken directly by the
    //! UART interrupt. After a successful start `schedIn` only publishes telemetry/events and
    //! nudges the task. On failure the driver logs, keeps draining from `schedIn`, and returns
    //! the task status so the topology can decide how to proceed. Call at most once, after
    //! `configure()`; if `configure()` did not accept the device, returns `INVALID_HANDLE` without
    //! creating a task. A failed `start()` may be retried.
    //!
    //! \param priority: task priority (Zephyr: 0 = highest, clamped to 14 by Os::Task)
    //! \param stackSize: task stack size in bytes (must be satisfiable by the Zephyr thread
    //!                   stack pool, e.g. <= CONFIG_DYNAMIC_THREAD_STACK_SIZE)
    //! \return Os::Task::OP_OK when the task is running; INVALID_HANDLE when no device is configured;
    //!         otherwise the Os::Task::start status
    Os::Task::Status start(FwTaskPriorityType priority, FwSizeType stackSize = Os::Task::TASK_DEFAULT);

    //! \brief Request the RX task (if started) to exit
    void stop();

    //! \brief Join the RX task (if started)
    Os::Task::Status join();

    //! \brief UART interrupt callback registered with Zephyr
    //!
    //! Runs in interrupt context for hardware UARTs and in the CDC ACM work-queue thread for USB
    //! CDC ACM. Touches only the ring buffers, atomics, the semaphore and the Zephyr UART API.
    //! \param dev: UART device
    //! \param user_data: `this`
    static void serial_cb(const struct device* dev, void* user_data);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for schedIn
    void schedIn_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;

    //! Handler implementation for send
    Drv::ByteStreamStatus send_handler(FwIndexType portNum,    //!< The port number
                                       Fw::Buffer& sendBuffer  //!< Frame to transmit (caller retains ownership)
                                       ) override;

    //! Handler implementation for recvReturnIn
    void recvReturnIn_handler(FwIndexType portNum,      //!< The port number
                              Fw::Buffer& returnBuffer  //!< Buffer returned from `recv`
                              ) override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! \brief Interrupt-side receive: move device FIFO bytes into the RX ring, pause when full
    //! \return true if at least one byte was received
    bool isrReceive();

    //! \brief Drain the RX ring into allocated buffers delivered on `recv` (task context)
    //!
    //! Bounded by MAX_DRAIN_ITERATIONS. Stops early when the allocator returns an empty buffer,
    //! leaving the data in the ring for the next drain. Must be called by a single consumer.
    void drainRx();

    //! \brief Re-enable the RX interrupt if it was paused and the ring has room again
    void resumeRxIfPaused();

    //! \brief Publish telemetry and any pending overrun/drop events (task context)
    void reportStatus();

    //! \brief Entry point of the optional RX task
    static void rxTaskEntry(void* ptr);

    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    //! Upper bound on ring claims per interrupt: a ring exposes at most two contiguous free
    //! regions, so two full claims fill it and a third claim returns 0 (ring full)
    static constexpr FwSizeType MAX_ISR_CLAIMS = 3;

    //! Upper bound on `recv` deliveries per drain: a full ring plus concurrent refill
    static constexpr FwSizeType MAX_DRAIN_ITERATIONS =
        (2 * ZephyrUartDriverCfg::RX_RING_SIZE) / ZephyrUartDriverCfg::RX_CHUNK_SIZE + 1;

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    const struct device* m_dev;  //!< UART device, nullptr until configure()

    struct ring_buf m_rxRing;                            //!< RX ring (producer: ISR, consumer: drain)
    U8 m_rxRingData[ZephyrUartDriverCfg::RX_RING_SIZE];  //!< RX ring storage

    struct k_sem m_wakeSem;  //!< Given by the ISR (and schedIn) to wake the RX task
    Os::Task m_rxTask;       //!< Optional event-driven RX task

    std::atomic<bool> m_taskStarted;  //!< RX task owns the RX ring; schedIn must not drain
    std::atomic<bool> m_quit;         //!< RX task exit request
    std::atomic<bool> m_rxPaused;     //!< RX interrupt disabled because the ring was full

    std::atomic<U32> m_rxBytes;       //!< Bytes moved from the device into the RX ring (U32: 64-bit
                                      //!< atomics are not lock-free on Cortex-M)
    std::atomic<U32> m_rxOverruns;    //!< Hardware overrun indications + ring put failures
    std::atomic<U32> m_rxPauseCount;  //!< Back-pressure engagements

    std::atomic<U32> m_rxAllocFails;  //!< Drain stalls for lack of an Fw::Buffer (drain writes, schedIn reads)
    U32 m_rxOverrunsReported;         //!< Overruns already reported via event (schedIn context only)
};

}  // end namespace Zephyr

#endif
