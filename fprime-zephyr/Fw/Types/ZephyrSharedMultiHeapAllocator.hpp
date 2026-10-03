// ======================================================================
// \title  ZephyrSharedMultiHeapAllocator.hpp
// \brief  Fw::MemAllocator backed by Zephyr's shared_multi_heap pool
// ======================================================================
#ifndef Zephyr_ZephyrSharedMultiHeapAllocator_HPP
#define Zephyr_ZephyrSharedMultiHeapAllocator_HPP

// shared_multi_heap.h relies on kernel.h for its integer types; multi_heap.h has no extern "C" guard of its own
#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>
#include <Fw/Types/ByteArray.hpp>
#include <Fw/Types/MemAllocator.hpp>
extern "C" {
#include <zephyr/sys/multi_heap.h>
}

namespace Zephyr {

//! \brief Fw::MemAllocator over Zephyr's shared_multi_heap pool (CONFIG_SHARED_MULTI_HEAP=y) for one attribute
//! Calls through all instances share one spinlock; direct shared_multi_heap_* callers are not serialized with them
class ZephyrSharedMultiHeapAllocator : public Fw::MemAllocator {
  public:
    //! Maximum regions in the shared pool, across all attributes, including regions added by board/SoC code
    static constexpr FwSizeType MAX_REGIONS = MAX_MULTI_HEAPS;

    enum Status {
        OP_OK,            //!< Region added
        INVALID_REGION,   //!< Region is null, too small, too large for sys_heap, or overlaps a region from addRegion()
        INVALID_ATTR,     //!< Attribute is out of range
        NO_MORE_REGIONS,  //!< MAX_REGIONS added through addRegion() (board/SoC regions not counted) or attribute full
    };

    //! Construct an allocator drawing memory with the given attribute
    explicit ZephyrSharedMultiHeapAllocator(shared_multi_heap_attr attr = SMH_REG_ATTR_CACHEABLE);

    ~ZephyrSharedMultiHeapAllocator() override = default;

    //! Add a RAM region with attr to the shared pool, initializing the pool if needed
    static Status addRegion(shared_multi_heap_attr attr, const Fw::ByteArray& region);

    //! Allocate size bytes at alignment (power of two) from the pool. size is set to 0 on failure.
    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment = alignof(std::max_align_t)) override;

    //! Return memory from allocate() to the pool
    void deallocate(const FwEnumStoreType identifier, void* ptr) override;

    ZephyrSharedMultiHeapAllocator(const ZephyrSharedMultiHeapAllocator&) = delete;
    ZephyrSharedMultiHeapAllocator& operator=(const ZephyrSharedMultiHeapAllocator&) = delete;

  private:
    const shared_multi_heap_attr m_attr;
};

}  // namespace Zephyr

#endif  // Zephyr_ZephyrSharedMultiHeapAllocator_HPP
