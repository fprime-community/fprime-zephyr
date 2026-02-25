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

        FW_ASSERT(size <= std::numeric_limits<size_t>::max(), static_cast<FwAssertArgType>(size));
        void* memory = k_aligned_alloc(static_cast<size_t>(alignment), static_cast<size_t>(size));
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
