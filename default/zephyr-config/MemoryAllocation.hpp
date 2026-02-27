// ======================================================================
// \title  config/MemoryAllocation.hpp
// \brief  Zephyr memory allocator configuration
// ======================================================================
#ifndef CONFIG_MEMORY_ALLOCATION_HPP
#define CONFIG_MEMORY_ALLOCATION_HPP

#include <Fw/Types/Assert.hpp>
#include <Fw/Types/MemAllocator.hpp>
#include <limits>
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

        // k_aligned_alloc requires alignment >= sizeof(void*) and power-of-two
        const FwSizeType minAlignment = static_cast<FwSizeType>(sizeof(void*));
        FwSizeType safeAlignment = (alignment < minAlignment) ? minAlignment : alignment;
        // Round up to next power of two if needed
        if ((safeAlignment & (safeAlignment - 1U)) != 0U) {
            safeAlignment--;
            safeAlignment |= safeAlignment >> 1;
            safeAlignment |= safeAlignment >> 2;
            safeAlignment |= safeAlignment >> 4;
            safeAlignment |= safeAlignment >> 8;
            safeAlignment |= safeAlignment >> 16;
            safeAlignment++;
        }

        // k_aligned_alloc (C11 aligned_alloc) requires size to be a multiple of alignment
        const FwSizeType remainder = size % safeAlignment;
        const FwSizeType allocSize = (remainder == 0U) ? size : (size + safeAlignment - remainder);

        void* memory = k_aligned_alloc(static_cast<size_t>(safeAlignment), static_cast<size_t>(allocSize));
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
