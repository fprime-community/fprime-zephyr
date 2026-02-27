// Mock fprime MemAllocator for host-side testing
#ifndef FW_TYPES_MEMALLOCATOR_HPP
#define FW_TYPES_MEMALLOCATOR_HPP

#include <cstddef>
#include <cstdint>
#include <new>

using FwEnumStoreType = int32_t;
using FwSizeType = size_t;

namespace Fw {

class MemAllocator {
  public:
    virtual ~MemAllocator() = default;

    virtual void* allocate(FwEnumStoreType identifier,
                           FwSizeType& size,
                           bool& recoverable,
                           FwSizeType alignment = alignof(std::max_align_t)) = 0;

    virtual void deallocate(FwEnumStoreType identifier, void* ptr) = 0;
};

}  // namespace Fw

#endif  // FW_TYPES_MEMALLOCATOR_HPP
