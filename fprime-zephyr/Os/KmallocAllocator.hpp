#ifndef FPRIME_ZEPHYR_OS_KMALLOCALLOCATOR_HPP
#define FPRIME_ZEPHYR_OS_KMALLOCALLOCATOR_HPP

#include <Fw/Types/MemAllocator.hpp>

#include <cstddef>

namespace Zephyr {

//! \class KmallocAllocator
//! \brief Fw::MemAllocator implementation backed by Zephyr's system heap.
//!
//! Routes allocations through k_malloc / k_aligned_alloc and releases them
//! via k_free. Requires CONFIG_HEAP_MEM_POOL_SIZE to be set in the
//! deployment's prj.conf.
class KmallocAllocator final : public Fw::MemAllocator {
  public:
    KmallocAllocator() = default;
    ~KmallocAllocator() override = default;

    void* allocate(FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment = alignof(std::max_align_t)) override;

    void deallocate(FwEnumStoreType identifier, void* ptr) override;

  private:
    static bool isPowerOfTwo(FwSizeType value);
    static FwSizeType normalizeAlignment(FwSizeType alignment);
};

}  // namespace Zephyr

#endif
