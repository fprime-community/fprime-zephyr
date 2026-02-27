// ======================================================================
// \title  config/MemoryAllocation.hpp
// \brief  Zephyr memory allocator using k_malloc/k_aligned_alloc
// ======================================================================
#ifndef CONFIG_MEMORY_ALLOCATION_HPP
#define CONFIG_MEMORY_ALLOCATION_HPP

#include <Fw/Types/MemAllocator.hpp>
#include <zephyr/kernel.h>

namespace Fw {

class ZephyrKmallocAllocator final : public MemAllocator {
  public:
    ZephyrKmallocAllocator() = default;
    ~ZephyrKmallocAllocator() override = default;

    void* allocate(const FwEnumStoreType identifier,
                   FwSizeType& size,
                   bool& recoverable,
                   FwSizeType alignment = alignof(std::max_align_t)) override {
        static_cast<void>(identifier);
        recoverable = false;

        // k_aligned_alloc requires alignment to be a power of two.
        // Round up if needed to prevent __ASSERT crash.
        FwSizeType safeAlignment = alignment;
        if (safeAlignment == 0U) {
            safeAlignment = 1U;
        }
        if ((safeAlignment & (safeAlignment - 1U)) != 0U) {
            safeAlignment--;
            safeAlignment |= safeAlignment >> 1;
            safeAlignment |= safeAlignment >> 2;
            safeAlignment |= safeAlignment >> 4;
            safeAlignment |= safeAlignment >> 8;
            safeAlignment |= safeAlignment >> 16;
            safeAlignment++;
        }

        // Use k_malloc for default/small alignments (simpler, no over-allocation).
        // Use k_aligned_alloc only when a larger alignment is requested.
        const FwSizeType minAlignment = static_cast<FwSizeType>(sizeof(void*));
        void* memory = (safeAlignment <= minAlignment)
                            ? k_malloc(static_cast<size_t>(size))
                            : k_aligned_alloc(static_cast<size_t>(safeAlignment), static_cast<size_t>(size));
        if (memory == nullptr) {
            size = 0;
        }
        return memory;
    }

    void deallocate(const FwEnumStoreType identifier, void* ptr) override {
        static_cast<void>(identifier);
        k_free(ptr);
    }
};

namespace MemoryAllocation {
using DefaultMemoryAllocatorType = Fw::ZephyrKmallocAllocator;
}  // namespace MemoryAllocation
}  // namespace Fw

#endif  // CONFIG_MEMORY_ALLOCATION_HPP
