// ======================================================================
// \title  ZephyrTouchReset.cpp
// \brief  cpp file for ZephyrTouchReset component implementation class
// ======================================================================

#include "fprime-zephyr/Svc/ZephyrTouchReset/ZephyrTouchReset.hpp"
#include <Fw/Logger/Logger.hpp>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/reboot.h>
#if defined(CONFIG_RETENTION_BOOT_MODE)
#include <zephyr/retention/bootmode.h>
#endif

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrTouchReset ::ZephyrTouchReset(const char* const compName)
    : ZephyrTouchResetComponentBase(compName), m_device(nullptr), m_touchBaud(DEFAULT_TOUCH_BAUD) {}

ZephyrTouchReset ::~ZephyrTouchReset() {}

void ZephyrTouchReset ::configure(const struct device* device, U32 touchBaud) {
    this->m_device = device;
    this->m_touchBaud = touchBaud;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ZephyrTouchReset ::run_handler(FwIndexType portNum, U32 context) {
    if ((this->m_device == nullptr) || !device_is_ready(this->m_device)) {
        return;
    }
    U32 baud = 0;
    if ((uart_line_ctrl_get(this->m_device, UART_LINE_CTRL_BAUD_RATE, &baud) == 0) && (baud == this->m_touchBaud)) {
        this->enterBootloader();
    }
}

void ZephyrTouchReset ::enterBootloader() {
    Fw::Logger::log("Touch baud %" PRIu32 " detected, rebooting into bootloader\n", this->m_touchBaud);
#if defined(CONFIG_RETENTION_BOOT_MODE)
    // Boot mode is retained across the warm reboot and consumed by the bootloader / SoC early init
    (void)bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
#endif
    sys_reboot(SYS_REBOOT_WARM);
}

}  // namespace Zephyr
