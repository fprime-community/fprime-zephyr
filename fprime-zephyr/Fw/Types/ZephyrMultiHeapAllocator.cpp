// ======================================================================
// \title  ZephyrMultiHeapAllocator.cpp
// \brief  Fw::MemAllocator over a Zephyr sys_multi_heap
// ======================================================================
#include "fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp"
#include <Fw/Types/Assert.hpp>

namespace Zephyr {

ZephyrMultiHeapAllocator::ZephyrMultiHeapAllocator()
    : Fw::MemAllocator(),
      m_ownedMultiHeap{},
      m_heaps{},
      m_ranges{},
      m_regionCount(0),
      m_multiHeap(m_ownedMultiHeap),
      m_cfg(this),
      m_owned(true),
      m_lock{} {
    sys_multi_heap_init(&m_ownedMultiHeap, firstFitChoice);
}

ZephyrMultiHeapAllocator::ZephyrMultiHeapAllocator(sys_multi_heap& multiHeap, void* cfg)
    : Fw::MemAllocator(),
      m_ownedMultiHeap{},
      m_heaps{},
      m_ranges{},
      m_regionCount(0),
      m_multiHeap(multiHeap),
      m_cfg(cfg),
      m_owned(false),
      m_lock{} {}

ZephyrMultiHeapAllocator::Status ZephyrMultiHeapAllocator::addRegion(const Fw::ByteArray& region) {
    if (not m_owned) {
        return EXTERNAL_HEAP;
    }
    if (not HeapRegion::isValid(region)) {
        return INVALID_REGION;
    }
    const HeapRegion::Range range = HeapRegion::toRange(region);
    Status status = OP_OK;
    k_spinlock_key_t key = k_spin_lock(&m_lock);
    if (m_regionCount >= MAX_REGIONS) {
        status = NO_MORE_REGIONS;
    } else if (HeapRegion::overlapsAny(range, m_ranges, m_regionCount)) {
        status = INVALID_REGION;
    } else {
        sys_heap& heap = m_heaps[m_regionCount];
        sys_heap_init(&heap, region.bytes, static_cast<size_t>(region.size));
        sys_multi_heap_add_heap(&m_ownedMultiHeap, &heap, nullptr);
        m_ranges[m_regionCount] = range;
        m_regionCount++;
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
    void* memory = nullptr;
    if (HeapRegion::isAllocatable(size, alignment)) {
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
