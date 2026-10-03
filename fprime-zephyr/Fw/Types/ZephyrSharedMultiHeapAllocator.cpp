// ======================================================================
// \title  ZephyrSharedMultiHeapAllocator.cpp
// \brief  Fw::MemAllocator over Zephyr's shared_multi_heap pool
// ======================================================================
#include <Fw/Types/Assert.hpp>
#include <limits>
#include "fprime-zephyr/Fw/Types/ZephyrHeapRegion.hpp"
#include "fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp"

namespace Zephyr {

namespace {
//! Serializes access to the system-wide shared_multi_heap pool across allocator instances
k_spinlock s_sharedLock;
}  // namespace

ZephyrSharedMultiHeapAllocator::ZephyrSharedMultiHeapAllocator(shared_multi_heap_attr attr) : m_attr(attr) {
    FW_ASSERT(attr < MAX_SHARED_MULTI_HEAP_ATTR, static_cast<FwAssertArgType>(attr));
}

ZephyrSharedMultiHeapAllocator::Status ZephyrSharedMultiHeapAllocator::addRegion(shared_multi_heap_attr attr,
                                                                                 const Fw::ByteArray& region) {
    if (attr >= MAX_SHARED_MULTI_HEAP_ATTR) {
        return INVALID_ATTR;
    }
    if (not HeapRegion::isValid(region, ZephyrMultiHeapAllocator::MIN_REGION_SIZE)) {
        return INVALID_REGION;
    }
    shared_multi_heap_region smhRegion = {static_cast<uint32_t>(attr), reinterpret_cast<uintptr_t>(region.bytes),
                                          static_cast<size_t>(region.size)};
    k_spinlock_key_t key = k_spin_lock(&s_sharedLock);
    // -EALREADY indicates board/SoC code or a previous call initialized the pool
    const int initStatus = shared_multi_heap_pool_init();
    FW_ASSERT((initStatus == 0) or (initStatus == -EALREADY), static_cast<FwAssertArgType>(initStatus));
    const int addStatus = shared_multi_heap_add(&smhRegion, nullptr);
    k_spin_unlock(&s_sharedLock, key);
    if (addStatus == -ENOMEM) {
        return NO_MORE_REGIONS;
    }
    FW_ASSERT(addStatus == 0, static_cast<FwAssertArgType>(addStatus));
    return OP_OK;
}

void* ZephyrSharedMultiHeapAllocator::allocate(const FwEnumStoreType identifier,
                                               FwSizeType& size,
                                               bool& recoverable,
                                               FwSizeType alignment) {
    (void)identifier;
    recoverable = false;
    FW_ASSERT((alignment & (alignment - 1)) == 0, static_cast<FwAssertArgType>(alignment));
    void* memory = nullptr;
    if ((size > 0) and (size <= std::numeric_limits<size_t>::max())) {
        k_spinlock_key_t key = k_spin_lock(&s_sharedLock);
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
