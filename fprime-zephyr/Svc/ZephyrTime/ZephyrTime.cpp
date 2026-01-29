// ======================================================================
// \title  ZephyrTime.cpp
// \author mstarch
// \brief  cpp file for ZephyrTime component implementation class
// ======================================================================

#include "fprime-zephyr/Svc/ZephyrTime/ZephyrTime.hpp"
#include <zephyr/sys/clock.h>
#include <zephyr/kernel.h>
#include <sys/_timespec.h>

namespace Zephyr {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ZephyrTime ::ZephyrTime(const char* const compName) : ZephyrTimeComponentBase(compName) {}

ZephyrTime ::~ZephyrTime() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ZephyrTime ::timeGetPort_handler(FwIndexType portNum, Fw::Time& time) {
    // Read the clock time
    struct timespec time_spec;
    int status = sys_clock_gettime(SYS_CLOCK_REALTIME, &time_spec);
    if (status == 0) {
        time.set(static_cast<U32>(time_spec.tv_sec), static_cast<U32>(time_spec.tv_nsec / 1000));
    } else {
        // Get uptime in milliseconds as fallback
        int64_t uptime = k_uptime_get();
        time.set(static_cast<U32>(uptime / 1000), static_cast<U32>((uptime % 1000) * 1000));
    }
}

}  // namespace Zephyr
