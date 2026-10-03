// ======================================================================
// \title  ZephyrHeapRegion.hpp
// \brief  Validation of RAM regions handed to Zephyr's sys_heap_init
// ======================================================================
#ifndef Zephyr_ZephyrHeapRegion_HPP
#define Zephyr_ZephyrHeapRegion_HPP

#include <Fw/Types/ByteArray.hpp>
#include <limits>

namespace Zephyr {
namespace HeapRegion {

//! sys_heap chunk unit and maximum chunk count (see zephyr/lib/heap/heap.h)
constexpr FwSizeType CHUNK_UNIT = 8;
#if defined(CONFIG_SYS_HEAP_SMALL_ONLY)
constexpr FwSizeType MAX_CHUNKS = 0x7fffU;
#else
constexpr FwSizeType MAX_CHUNKS = 0x7fffffffU;
#endif

//! True when region can be passed to sys_heap_init without tripping its size assertions
inline bool isValid(const Fw::ByteArray& region, const FwSizeType minimumSize) {
    return (region.bytes != nullptr) and (region.size >= minimumSize) and
           (region.size <= std::numeric_limits<size_t>::max()) and ((region.size / CHUNK_UNIT) <= MAX_CHUNKS);
}

}  // namespace HeapRegion
}  // namespace Zephyr

#endif  // Zephyr_ZephyrHeapRegion_HPP
