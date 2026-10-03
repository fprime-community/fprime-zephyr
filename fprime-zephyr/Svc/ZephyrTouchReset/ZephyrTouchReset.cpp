// ======================================================================
// \title  ZephyrTouchReset.cpp
// \brief  cpp file for ZephyrTouchReset component implementation class
// ======================================================================

#include "fprime-zephyr/Svc/ZephyrTouchReset/ZephyrTouchReset.hpp"
#include <Fw/Logger/Logger.hpp>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrTouchReset ::ZephyrTouchReset(const char* const compName)
    : ZephyrTouchResetComponentBase(compName),
      m_pollWork(),
      m_device(nullptr),
      m_entry(Bootloader::enter),
      m_touchBaud(Bootloader::TOUCH_BAUD),
      m_pollPeriodMs(DEFAULT_POLL_PERIOD_MS),
      m_armed(false) {
    this->m_pollWork.component = this;
    k_work_init_delayable(&this->m_pollWork.work, ZephyrTouchReset::pollHandler);
}

ZephyrTouchReset ::~ZephyrTouchReset() {
    struct k_work_sync sync{};
    (void)k_work_cancel_delayable_sync(&this->m_pollWork.work, &sync);
}

Fw::Success ZephyrTouchReset ::configure(const struct device* device,
                                         U32 touchBaud,
                                         Bootloader::EntryFunction entry,
                                         U32 pollPeriodMs) {
    FW_ASSERT(entry != nullptr);
    FW_ASSERT(pollPeriodMs > 0);
    struct k_work_sync sync{};
    (void)k_work_cancel_delayable_sync(&this->m_pollWork.work, &sync);
    this->m_device = nullptr;

    if ((device == nullptr) || !device_is_ready(device)) {
        Fw::Logger::log("[TouchReset] UART device not ready, touch reset disabled\n");
        return Fw::Success::FAILURE;
    }
    U32 baud = 0;
    const int status = uart_line_ctrl_get(device, UART_LINE_CTRL_BAUD_RATE, &baud);
    if (status != 0) {
        Fw::Logger::log("[TouchReset] UART baud rate unreadable (%d), touch reset disabled\n", status);
        return Fw::Success::FAILURE;
    }
    this->m_touchBaud = touchBaud;
    this->m_entry = entry;
    this->m_pollPeriodMs = pollPeriodMs;
    this->m_armed = false;
    this->m_device = device;
    (void)k_work_reschedule(&this->m_pollWork.work, K_MSEC(this->m_pollPeriodMs));
    return Fw::Success::SUCCESS;
}

void ZephyrTouchReset ::check() {
    if ((this->m_device == nullptr) || !device_is_ready(this->m_device)) {
        return;
    }
    U32 baud = 0;
    if (uart_line_ctrl_get(this->m_device, UART_LINE_CTRL_BAUD_RATE, &baud) != 0) {
        return;
    }
    if (baud != this->m_touchBaud) {
        this->m_armed = true;
    } else if (this->m_armed) {
        // printk keeps the system work queue stack small (Fw::Logger formats into a stack Fw::String)
        printk("[TouchReset] %" PRIu32 " baud touch, entering bootloader: %s\n", this->m_touchBaud, Bootloader::METHOD);
        this->m_entry();
    }
}

void ZephyrTouchReset ::pollHandler(struct k_work* work) {
    struct k_work_delayable* delayable = k_work_delayable_from_work(work);
    PollWork* pollWork = CONTAINER_OF(delayable, PollWork, work);
    ZephyrTouchReset* component = pollWork->component;
    component->check();
    (void)k_work_schedule(&pollWork->work, K_MSEC(component->m_pollPeriodMs));
}

}  // namespace Zephyr
