// ======================================================================
// \title  ZephyrMultiHeapAllocator.hpp
// \brief  Fw::MemAllocator backed by a Zephyr sys_multi_heap
// ======================================================================
#ifndef Zephyr_ZephyrMultiHeapAllocator_HPP
#define Zephyr_ZephyrMultiHeapAllocator_HPP

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_heap.h>
#include <Fw/Types/ByteArray.hpp>
#include <Fw/Types/MemAllocator.hpp>
#include "fprime-zephyr/Fw/Types/ZephyrHeapRegion.hpp"
// multi_heap.h has no extern "C" guard of its own
extern "C" {
#include <zephyr/sys/multi_heap.h>
}

namespace Zephyr {

//! \brief Fw::MemAllocator over a Zephyr sys_multi_heap (CONFIG_MULTI_HEAP=y); first-fit over regions in add order
//! Calls through one instance are spinlock-serialized; see docs/sdd.md for wrapping an external sys_multi_heap
class ZephyrMultiHeapAllocator : public Fw::MemAllocator {
  public:
    //! Maximum number of regions in a single multi-heap
    static constexpr FwSizeType MAX_REGIONS = MAX_MULTI_HEAPS;
    //! Smallest region accepted by addRegion()
    static constexpr FwSizeType MIN_REGION_SIZE = HeapRegion::MIN_REGION_SIZE;

    enum Status {
        OP_OK,            //!< Region added
        INVALID_REGION,   //!< Region is null, too small, too large for sys_heap, or overlaps an added region
        NO_MORE_REGIONS,  //!< MAX_REGIONS have already been added
        EXTERNAL_HEAP,    //!< Allocator wraps an external sys_multi_heap; add heaps to it directly
    };

    //! Construct an allocator owning an empty multi-heap
    ZephyrMultiHeapAllocator();

    //! Construct an allocator wrapping an initialized multi-heap; cfg is passed to its choice function
    ZephyrMultiHeapAllocator(sys_multi_heap& multiHeap, void* cfg);

    ~ZephyrMultiHeapAllocator() override = default;

    //! Add a RAM region to the owned multi-heap; the region must stay valid for the allocator's lifetime
    Status addRegion(const Fw::ByteArray& region);

    //! Allocate size bytes at alignment (power of two) from the multi-heap. size is set to 0 on failure.
    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment = alignof(std::max_align_t)) override;

    //! Return memory from allocate() to the multi-heap
    void deallocate(const FwEnumStoreType identifier, void* ptr) override;

    ZephyrMultiHeapAllocator(const ZephyrMultiHeapAllocator&) = delete;
    ZephyrMultiHeapAllocator& operator=(const ZephyrMultiHeapAllocator&) = delete;

  private:
    //! First-fit choice function for the owned multi-heap; cfg is the owning allocator
    static void* firstFitChoice(sys_multi_heap* multiHeap, void* cfg, size_t align, size_t size);

    sys_multi_heap m_ownedMultiHeap;
    sys_heap m_heaps[MAX_REGIONS];
    HeapRegion::Range m_ranges[MAX_REGIONS];
    FwSizeType m_regionCount;
    sys_multi_heap& m_multiHeap;
    void* const m_cfg;
    const bool m_owned;
    k_spinlock m_lock;
};

}  // namespace Zephyr

#endif  // Zephyr_ZephyrMultiHeapAllocator_HPP
