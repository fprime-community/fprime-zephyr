// ======================================================================
// \title  ZephyrSharedMultiHeapAllocator.cpp
// \brief  Fw::MemAllocator over Zephyr's shared_multi_heap pool
// ======================================================================
#include "fprime-zephyr/Fw/Types/ZephyrSharedMultiHeapAllocator.hpp"
#include <zephyr/kernel.h>
#include <Fw/Types/Assert.hpp>
#include <cerrno>
#include <cstdint>
#include <limits>

namespace Zephyr {

namespace {
//! sys_heap chunk unit and maximum chunk count (see zephyr/lib/heap/heap.h)
constexpr FwSizeType CHUNK_UNIT = 8;
#if defined(CONFIG_SYS_HEAP_SMALL_ONLY)
constexpr FwSizeType MAX_CHUNKS = 0x7fffU;
#else
constexpr FwSizeType MAX_CHUNKS = 0x7fffffffU;
#endif

//! Address range of a region already added to the pool
struct Range {
    uintptr_t start;
    uintptr_t end;
};

//! True when region can be passed to sys_heap_init without tripping its size assertions
bool isValid(const Fw::ByteArray& region) {
    return (region.bytes != nullptr) and (region.size >= ZephyrSharedMultiHeapAllocator::MIN_REGION_SIZE) and
           (region.size <= std::numeric_limits<size_t>::max()) and ((region.size / CHUNK_UNIT) <= MAX_CHUNKS) and
           // reinterpret_cast: address arithmetic on unrelated regions needs integers, not pointer comparisons
           (reinterpret_cast<uintptr_t>(region.bytes) <= (std::numeric_limits<uintptr_t>::max() - region.size));
}

//! Range covered by a valid region
Range toRange(const Fw::ByteArray& region) {
    // reinterpret_cast: ranges are compared as integers; shared_multi_heap_region::addr is also a uintptr_t
    const uintptr_t start = reinterpret_cast<uintptr_t>(region.bytes);
    return Range{start, start + static_cast<uintptr_t>(region.size)};
}

//! True when an allocate() request can be passed to Zephyr unchanged; asserts alignment is a power of two
bool isAllocatable(const FwSizeType size, const FwSizeType alignment) {
    FW_ASSERT((alignment & (alignment - 1)) == 0, static_cast<FwAssertArgType>(alignment));
    return (size > 0) and (size <= std::numeric_limits<size_t>::max()) and
           (alignment <= std::numeric_limits<size_t>::max());
}

//! Serializes access to the system-wide shared_multi_heap pool across allocator instances
k_spinlock s_sharedLock;
//! True once ensurePoolReady() has confirmed the pool is initialized (by it or, via -EALREADY, by board/SoC code)
bool s_poolReady = false;
//! Regions added through addRegion(); Zephyr does not expose regions added by board/SoC code
Range s_ranges[ZephyrSharedMultiHeapAllocator::MAX_REGIONS];
FwSizeType s_regionCount = 0;

//! True when region intersects a region already added through addRegion(); call with s_sharedLock held
bool overlapsAdded(const Range& region) {
    bool overlaps = false;
    for (FwSizeType i = 0; (i < s_regionCount) and (not overlaps); i++) {
        overlaps = (region.start < s_ranges[i].end) and (s_ranges[i].start < region.end);
    }
    return overlaps;
}

//! Initialize the pool if needed so its choice function is set; call with s_sharedLock held
void ensurePoolReady() {
    if (not s_poolReady) {
        // -EALREADY indicates board/SoC code initialized the pool
        const int status = shared_multi_heap_pool_init();
        FW_ASSERT((status == 0) or (status == -EALREADY), static_cast<FwAssertArgType>(status));
        s_poolReady = true;
    }
}
}  // namespace

ZephyrSharedMultiHeapAllocator::ZephyrSharedMultiHeapAllocator(shared_multi_heap_attr attr)
    : Fw::MemAllocator(), m_attr(attr) {
    FW_ASSERT(attr < MAX_SHARED_MULTI_HEAP_ATTR, static_cast<FwAssertArgType>(attr));
}

ZephyrSharedMultiHeapAllocator::Status ZephyrSharedMultiHeapAllocator::addRegion(shared_multi_heap_attr attr,
                                                                                 const Fw::ByteArray& region) {
    if (attr >= MAX_SHARED_MULTI_HEAP_ATTR) {
        return INVALID_ATTR;
    }
    if (not isValid(region)) {
        return INVALID_REGION;
    }
    const Range range = toRange(region);
    shared_multi_heap_region smhRegion = {};
    smhRegion.attr = static_cast<uint32_t>(attr);
    smhRegion.addr = range.start;
    smhRegion.size = static_cast<size_t>(region.size);

    Status status = OP_OK;
    int addStatus = 0;
    k_spinlock_key_t key = k_spin_lock(&s_sharedLock);
    // Zephyr checks only the per-attribute count; the pool's heap array is shared by all attributes
    if (s_regionCount >= MAX_REGIONS) {
        status = NO_MORE_REGIONS;
    } else if (overlapsAdded(range)) {
        status = INVALID_REGION;
    } else {
        ensurePoolReady();
        addStatus = shared_multi_heap_add(&smhRegion, nullptr);
        if (addStatus == 0) {
            s_ranges[s_regionCount] = range;
            s_regionCount++;
        } else if (addStatus == -ENOMEM) {
            status = NO_MORE_REGIONS;
        } else {
            status = INVALID_REGION;
        }
    }
    k_spin_unlock(&s_sharedLock, key);
    FW_ASSERT((addStatus == 0) or (addStatus == -ENOMEM), static_cast<FwAssertArgType>(addStatus));
    return status;
}

void* ZephyrSharedMultiHeapAllocator::allocate(const FwEnumStoreType identifier,
                                               FwSizeType& size,
                                               bool& recoverable,
                                               FwSizeType alignment) {
    (void)identifier;
    recoverable = false;
    void* memory = nullptr;
    if (isAllocatable(size, alignment)) {
        k_spinlock_key_t key = k_spin_lock(&s_sharedLock);
        ensurePoolReady();
        memory = shared_multi_heap_aligned_alloc(m_attr, static_cast<size_t>(alignment), static_cast<size_t>(size));
        k_spin_unlock(&s_sharedLock, key);
    }
    if (memory == nullptr) {
        size = 0;
    }
    return memory;
}

void ZephyrSharedMultiHeapAllocator::deallocate(const FwEnumStoreType identifier, void* ptr) {
    (void)identifier;
    k_spinlock_key_t key = k_spin_lock(&s_sharedLock);
    shared_multi_heap_free(ptr);
    k_spin_unlock(&s_sharedLock, key);
}

}  // namespace Zephyr
