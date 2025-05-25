#include <Os/IntervalTimer.hpp>
#include <Fw/Types/Assert.hpp>
#include <zephyr/kernel.h>

// Super shady, needs to be reworked. The precision is not Usec and not
// tested at all.
namespace Os {
    void IntervalTimer::getRawTime(RawTime& time) {
        int64_t uptime_ms = k_uptime_get();
        FW_ASSERT(uptime_ms > 0, uptime_ms);
        time.upper = uptime_ms >> 32;
        time.lower = uptime_ms & 0xFFFFFFFF;
    }

    U32 IntervalTimer::getDiffUsec(const RawTime& t1In, const RawTime& t2In) {
        int64_t t1 = ((int64_t)t1In.upper << 32) | t1In.lower;
        int64_t t2 = ((int64_t)t2In.upper << 32) | t2In.lower;
        return static_cast<U32>(t1 - t2) * 1000;
    }
}