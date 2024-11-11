// ======================================================================
// \title Os/Zephyr/RawTime.cpp
// \brief Zephyr implementation for Os::RawTime
// ======================================================================
#include "Zephyr/Os/RawTime.hpp"
#include <zephyr/kernel.h>

namespace Os {
namespace Zephyr {

//! \brief check is a is newer than b
bool isNewer(const ZephyrRawTimeHandle& a, const ZephyrRawTimeHandle& b) {
    return ((a.m_secs > b.m_secs) ||
           ((a.m_secs == b.m_secs) && (a.m_micros >= b.m_micros)));
}

RawTimeHandle* ZephyrRawTime::getHandle() {
    return &this->m_handle;
}

ZephyrRawTime::Status ZephyrRawTime::now() {
    U32 uptime_secs = k_uptime_seconds();
    U32 uptime_ms = k_uptime_get_32();
    
    this->m_handle.m_secs = uptime_secs;
    this->m_handle.m_micros = (uptime_ms % 1000) * 1000;
    return Status::OP_OK;
}

ZephyrRawTime::Status ZephyrRawTime::getTimeInterval(const Os::RawTime& other, Fw::TimeInterval& interval) const {
    interval.set(0, 0);
    const ZephyrRawTimeHandle& my_handle = this->m_handle;
    const ZephyrRawTimeHandle& other_handle = static_cast<const ZephyrRawTimeHandle&>(*const_cast<Os::RawTime&>(other).getHandle());

    const ZephyrRawTimeHandle& newer = isNewer(my_handle, other_handle) ? my_handle : other_handle;
    const ZephyrRawTimeHandle& older = isNewer(my_handle, other_handle) ? other_handle : my_handle;

    if (newer.m_micros < older.m_micros) {
        interval.set(newer.m_secs - older.m_secs - 1, 1000000 + newer.m_micros - older.m_micros);
    } else {
        interval.set(newer.m_secs - older.m_secs, newer.m_micros - older.m_micros);
    }

    return Status::OP_OK;
}

Fw::SerializeStatus ZephyrRawTime::serialize(Fw::SerializeBufferBase& buffer) const {
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_SERIALIZE_OK;
    status = buffer.serialize(this->m_handle.m_secs);
    if (status == Fw::FW_SERIALIZE_OK) {
        status = buffer.serialize(this->m_handle.m_micros);
    }
    return status;
}

Fw::SerializeStatus ZephyrRawTime::deserialize(Fw::SerializeBufferBase& buffer) {
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_SERIALIZE_OK;
    status = buffer.deserialize(this->m_handle.m_secs);
    if (status == Fw::FW_SERIALIZE_OK) {
        status = buffer.deserialize(this->m_handle.m_micros);
    }
    return status;
}
}  // namespace Zephyr
}  // namespace Os
