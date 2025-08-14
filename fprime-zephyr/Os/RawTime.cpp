// ======================================================================
// \title Os/Zephyre.cpp
// \brief Zephyrlementation for Os::RawTime
// Relevent documentation:
// https://docs.zephyrproject.org/latest/kernel/services/timing/clocks.html
// https://docs.zephyrproject.org/latest/doxygen/html/group__clock__apis.html
// ======================================================================
#include <Fw/Logger/Logger.hpp>
#include <Fw/Types/Assert.hpp>
#include <cerrno>
#include "fprime-zephyr/Os/RawTime.hpp"

namespace Os {
namespace Zephyr {
namespace RawTime {

bool isNewer(const ZephyrRawTimeHandle& a, const ZephyrRawTimeHandle& b) {
    return ((a.m_seconds > b.m_seconds) ||
           ((a.m_seconds == b.m_seconds) && (a.m_micros >= b.m_micros)));
}

ZephyrRawTime::Status ZephyrRawTime::now() {
    U32 uptime_millsec = k_uptime_get();
    this->m_handle.m_seconds = uptime_millsec / 1000;
    this->m_handle.m_micros = (uptime_millsec % 1000) * 1000;
    return Status::OP_OK;
}

ZephyrRawTime::Status ZephyrRawTime::getTimeInterval(const Os::RawTime& other, Fw::TimeInterval& interval) const {
    interval.set(0, 0);
    const ZephyrRawTimeHandle& my_handle = this->m_handle;
    const ZephyrRawTimeHandle& other_handle = static_cast<const ZephyrRawTimeHandle&>(*const_cast<Os::RawTime&>(other).getHandle());


    const ZephyrRawTimeHandle& newer = isNewer(my_handle, other_handle) ? my_handle : other_handle;
    const ZephyrRawTimeHandle& older = isNewer(my_handle, other_handle) ? other_handle : my_handle;
  
    if (newer.m_micros < older.m_micros) {
        interval.set(newer.m_seconds - older.m_seconds - 1, 1000000 + newer.m_micros - older.m_micros);
    } else {
        interval.set(newer.m_seconds - older.m_seconds, newer.m_micros - older.m_micros);
    }

    return Status::OP_OK;
}

Fw::SerializeStatus ZephyrRawTime::serializeTo(Fw::SerializeBufferBase& buffer) const {
    Fw::SerializeStatus status = buffer.serialize(static_cast<U32>(this->m_handle.m_seconds));
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return status;
    }
    return buffer.serialize(static_cast<U32>(this->m_handle.m_micros));
}

Fw::SerializeStatus ZephyrRawTime::deserializeFrom(Fw::SerializeBufferBase& buffer) {
    Fw::SerializeStatus status = buffer.deserialize(this->m_handle.m_seconds);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        return status;
    }
    return buffer.deserialize(this->m_handle.m_micros);
}

RawTimeHandle* ZephyrRawTime::getHandle() {
    return &this->m_handle;
}


}  // namespace RawTime
}  // namespace Zephyr
}  // namespace Os