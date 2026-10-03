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
    explicit ZephyrTouchReset(const char* const compName  //!< The component name
    );

    //! Destroy ZephyrTouchReset object, cancelling monitoring
    ~ZephyrTouchReset();

    //! The kernel holds a pointer to the work item, so the component cannot be copied
    ZephyrTouchReset(const ZephyrTouchReset&) = delete;
    ZephyrTouchReset& operator=(const ZephyrTouchReset&) = delete;

    //! Start monitoring a UART for the touch baud rate
    //!
    //! Monitoring runs on the Zephyr system work queue so that it keeps working when F Prime threads are starved.
    //! Defaults select the touch baud rate and bootloader entry method for the SoC / board being built. The
    //! bootloader is entered only when the baud rate changes to the touch baud rate after a different baud rate was
    //! observed. If the entry function returns, another change to the touch baud rate is needed to retry. Calling
    //! configure() again stops any previous monitoring first.
    //!
    //! \return SUCCESS when monitoring started, FAILURE (monitoring stopped) when the device is not ready or its baud
    //! rate cannot be read through uart_line_ctrl_get (e.g. CONFIG_UART_LINE_CTRL is disabled)
    Fw::Success configure(const struct device* device,  //!< UART device supporting line control (e.g. CDC ACM)
                          U32 touchBaud = Bootloader::TOUCH_BAUD,               //!< Baud rate that triggers the reboot
                          Bootloader::EntryFunction entry = Bootloader::enter,  //!< Reboots into the bootloader
                          U32 pollPeriodMs = DEFAULT_POLL_PERIOD_MS             //!< Period between baud rate checks
    );

  private:
    //! Check the monitored UART once and enter the bootloader when it changes to the touch baud rate
    void check();

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
    bool m_armed;  //!< A baud rate other than the touch baud rate has been observed
};

}  // namespace Zephyr

#endif
