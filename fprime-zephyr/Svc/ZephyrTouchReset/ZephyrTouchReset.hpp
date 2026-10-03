// ======================================================================
// \title  ZephyrTouchReset.hpp
// \brief  hpp file for ZephyrTouchReset component implementation class
// ======================================================================

#ifndef Zephyr_ZephyrTouchReset_HPP
#define Zephyr_ZephyrTouchReset_HPP

#include <Fw/Types/SuccessEnumAc.hpp>
#include "fprime-zephyr/Svc/ZephyrTouchReset/BootloaderEntry.hpp"
#include "fprime-zephyr/Svc/ZephyrTouchReset/ZephyrTouchResetComponentAc.hpp"

#include <zephyr/kernel.h>

namespace Zephyr {

class ZephyrTouchReset final : public ZephyrTouchResetComponentBase {
  public:
    //! Default period between baud rate checks
    static constexpr U32 DEFAULT_POLL_PERIOD_MS = 100;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ZephyrTouchReset object
    ZephyrTouchReset(const char* const compName  //!< The component name
    );

    //! Destroy ZephyrTouchReset object
    ~ZephyrTouchReset();

    //! Start monitoring a UART for the touch baud rate
    //!
    //! Monitoring runs on the Zephyr system work queue so that it keeps working when F Prime threads are starved.
    //! Defaults select the touch baud rate and bootloader entry method for the SoC / board being built.
    //!
    //! \return SUCCESS when monitoring started, FAILURE when the device is not ready or lacks CONFIG_UART_LINE_CTRL
    Fw::Success configure(const struct device* device,  //!< UART device supporting line control (e.g. CDC ACM)
                          U32 touchBaud = Bootloader::TOUCH_BAUD,               //!< Baud rate that triggers the reboot
                          Bootloader::EntryFunction entry = Bootloader::enter,  //!< Reboots into the bootloader
                          U32 pollPeriodMs = DEFAULT_POLL_PERIOD_MS             //!< Period between baud rate checks
    );

    //! Check the monitored UART once and enter the bootloader when it is set to the touch baud rate
    void check();

  private:
    //! Work item wrapper allowing the work handler to recover the component
    struct PollWork {
        struct k_work_delayable work;
        ZephyrTouchReset* component;
    };

    //! System work queue handler polling the UART
    static void pollHandler(struct k_work* work);

    PollWork m_pollWork;
    const struct device* m_device;
    Bootloader::EntryFunction m_entry;
    U32 m_touchBaud;
    U32 m_pollPeriodMs;
};

}  // namespace Zephyr

#endif
