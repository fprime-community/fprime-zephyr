// ======================================================================
// \title  ZephyrHeapRegion.hpp
// \brief  Region and request validation shared by the Zephyr multi-heap allocators
// ======================================================================
#ifndef Zephyr_ZephyrHeapRegion_HPP
#define Zephyr_ZephyrHeapRegion_HPP

#include <Fw/Types/Assert.hpp>
#include <Fw/Types/ByteArray.hpp>
#include <cstdint>
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
//! Smallest accepted region: covers the z_heap header, bucket array, and end marker with room for allocations
constexpr FwSizeType MIN_REGION_SIZE = 256;

//! Address range of a region already handed to a heap
struct Range {
    uintptr_t start;
    uintptr_t end;
};

//! True when region can be passed to sys_heap_init without tripping its size assertions
inline bool isValid(const Fw::ByteArray& region) {
    return (region.bytes != nullptr) and (region.size >= MIN_REGION_SIZE) and
           (region.size <= std::numeric_limits<size_t>::max()) and ((region.size / CHUNK_UNIT) <= MAX_CHUNKS) and
           (reinterpret_cast<uintptr_t>(region.bytes) <= (std::numeric_limits<uintptr_t>::max() - region.size));
}

//! Range covered by a valid region
inline Range toRange(const Fw::ByteArray& region) {
    const uintptr_t start = reinterpret_cast<uintptr_t>(region.bytes);
    return Range{start, start + static_cast<uintptr_t>(region.size)};
}

//! True when region intersects any of the first count ranges
inline bool overlapsAny(const Range& region, const Range* const ranges, const FwSizeType count) {
    bool overlaps = false;
    for (FwSizeType i = 0; (i < count) and (not overlaps); i++) {
        overlaps = (region.start < ranges[i].end) and (ranges[i].start < region.end);
    }
    return overlaps;
}

//! Common Fw::MemAllocator::allocate() pre-checks; true when the request can be passed to Zephyr unchanged
inline bool prepareAllocation(const FwSizeType size, bool& recoverable, const FwSizeType alignment) {
    recoverable = false;
    FW_ASSERT((alignment & (alignment - 1)) == 0, static_cast<FwAssertArgType>(alignment));
    return (size > 0) and (size <= std::numeric_limits<size_t>::max()) and
           (alignment <= std::numeric_limits<size_t>::max());
}

}  // namespace HeapRegion
}  // namespace Zephyr

#endif  // Zephyr_ZephyrHeapRegion_HPP
