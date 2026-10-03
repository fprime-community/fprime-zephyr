// ======================================================================
// \title  ZephyrMultiHeapAllocator.cpp
// \brief  Fw::MemAllocator over a Zephyr sys_multi_heap
// ======================================================================
#include "fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp"
#include <Fw/Types/Assert.hpp>
#include <limits>
#include "fprime-zephyr/Fw/Types/ZephyrHeapRegion.hpp"

namespace Zephyr {

ZephyrMultiHeapAllocator::ZephyrMultiHeapAllocator()
    : m_ownedMultiHeap{},
      m_heaps{},
      m_regionCount(0),
      m_multiHeap(m_ownedMultiHeap),
      m_cfg(this),
      m_owned(true),
      m_lock{} {
    sys_multi_heap_init(&m_ownedMultiHeap, firstFitChoice);
}

ZephyrMultiHeapAllocator::ZephyrMultiHeapAllocator(sys_multi_heap& multiHeap, void* cfg)
    : m_ownedMultiHeap{}, m_heaps{}, m_regionCount(0), m_multiHeap(multiHeap), m_cfg(cfg), m_owned(false), m_lock{} {}

ZephyrMultiHeapAllocator::Status ZephyrMultiHeapAllocator::addRegion(const Fw::ByteArray& region) {
    if (not m_owned) {
        return EXTERNAL_HEAP;
    }
    if (not HeapRegion::isValid(region, MIN_REGION_SIZE)) {
        return INVALID_REGION;
    }
    Status status = NO_MORE_REGIONS;
    k_spinlock_key_t key = k_spin_lock(&m_lock);
    if (m_regionCount < MAX_REGIONS) {
        sys_heap& heap = m_heaps[m_regionCount];
        sys_heap_init(&heap, region.bytes, static_cast<size_t>(region.size));
        sys_multi_heap_add_heap(&m_ownedMultiHeap, &heap, nullptr);
        m_regionCount++;
        status = OP_OK;
    }
    k_spin_unlock(&m_lock, key);
    return status;
}

void* ZephyrMultiHeapAllocator::allocate(const FwEnumStoreType identifier,
                                         FwSizeType& size,
                                         bool& recoverable,
                                         FwSizeType alignment) {
    (void)identifier;
    recoverable = false;
    FW_ASSERT((alignment & (alignment - 1)) == 0, static_cast<FwAssertArgType>(alignment));
    void* memory = nullptr;
    if ((size > 0) and (size <= std::numeric_limits<size_t>::max())) {
        k_spinlock_key_t key = k_spin_lock(&m_lock);
        memory = sys_multi_heap_aligned_alloc(&m_multiHeap, m_cfg, static_cast<size_t>(alignment),
                                              static_cast<size_t>(size));
        k_spin_unlock(&m_lock, key);
    }
    if (memory == nullptr) {
        size = 0;
    }
    return memory;
}

void ZephyrMultiHeapAllocator::deallocate(const FwEnumStoreType identifier, void* ptr) {
    (void)identifier;
    k_spinlock_key_t key = k_spin_lock(&m_lock);
    sys_multi_heap_free(&m_multiHeap, ptr);
    k_spin_unlock(&m_lock, key);
}

void* ZephyrMultiHeapAllocator::firstFitChoice(sys_multi_heap* multiHeap, void* cfg, size_t align, size_t size) {
    (void)multiHeap;
    FW_ASSERT(cfg != nullptr);
    ZephyrMultiHeapAllocator* const self = static_cast<ZephyrMultiHeapAllocator*>(cfg);
    void* memory = nullptr;
    for (FwSizeType i = 0; (i < self->m_regionCount) and (memory == nullptr); i++) {
        memory = sys_heap_aligned_alloc(&self->m_heaps[i], align, size);
    }
    return memory;
}

}  // namespace Zephyr
