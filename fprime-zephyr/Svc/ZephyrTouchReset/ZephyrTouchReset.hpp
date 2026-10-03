// ======================================================================
// \title  ZephyrTouchReset.hpp
// \brief  hpp file for ZephyrTouchReset component implementation class
// ======================================================================

#ifndef Zephyr_ZephyrTouchReset_HPP
#define Zephyr_ZephyrTouchReset_HPP

#include "fprime-zephyr/Svc/ZephyrTouchReset/ZephyrTouchResetComponentAc.hpp"

struct device;

namespace Zephyr {

class ZephyrTouchReset final : public ZephyrTouchResetComponentBase {
  public:
    //! Conventional baud rate used by hosts to request a reboot into the bootloader
    static constexpr U32 DEFAULT_TOUCH_BAUD = 1200;

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ZephyrTouchReset object
    ZephyrTouchReset(const char* const compName  //!< The component name
    );

    //! Destroy ZephyrTouchReset object
    ~ZephyrTouchReset();

    //! Configure the UART device to monitor and the baud rate that triggers the reboot
    void configure(const struct device* device,        //!< UART device supporting line control (e.g. CDC ACM)
                   U32 touchBaud = DEFAULT_TOUCH_BAUD  //!< Baud rate that triggers the reboot
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Polls the UART baud rate and reboots into the bootloader when it matches the touch baud rate
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    //! Reboot into the bootloader. Does not return.
    void enterBootloader();

    const struct device* m_device;
    U32 m_touchBaud;
};

}  // namespace Zephyr

#endif
