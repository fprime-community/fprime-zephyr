// ======================================================================
// \title  ZephyrSharedMultiHeapAllocator.cpp
// \brief  Fw::MemAllocator over Zephyr's shared_multi_heap pool
// ======================================================================
#include "fprime-zephyr/Fw/Types/ZephyrSharedMultiHeapAllocator.hpp"
#include <zephyr/kernel.h>
#include <Fw/Types/Assert.hpp>
#include <cerrno>
#include "fprime-zephyr/Fw/Types/ZephyrHeapRegion.hpp"

namespace Zephyr {

namespace {
//! Serializes access to the system-wide shared_multi_heap pool across allocator instances
k_spinlock s_sharedLock;
//! True once shared_multi_heap_pool_init() has run (here or in board/SoC code)
bool s_poolReady = false;
//! Regions added through addRegion(); Zephyr does not expose regions added by board/SoC code
HeapRegion::Range s_ranges[ZephyrSharedMultiHeapAllocator::MAX_REGIONS];
FwSizeType s_regionCount = 0;

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
    if (not HeapRegion::isValid(region)) {
        return INVALID_REGION;
    }
    const HeapRegion::Range range = HeapRegion::toRange(region);
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
    } else if (HeapRegion::overlapsAny(range, s_ranges, s_regionCount)) {
        status = INVALID_REGION;
    } else {
        ensurePoolReady();
        addStatus = shared_multi_heap_add(&smhRegion, nullptr);
        if (addStatus == 0) {
            s_ranges[s_regionCount] = range;
            s_regionCount++;
        } else if (addStatus == -ENOMEM) {
            status = NO_MORE_REGIONS;
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
    void* memory = nullptr;
    if (HeapRegion::prepareAllocation(size, recoverable, alignment)) {
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
