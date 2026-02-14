#ifndef FW_TYPES_MEMALLOCATOR_HPP
#define FW_TYPES_MEMALLOCATOR_HPP

#include <Fw/FPrimeBasicTypes.hpp>
#include <cstddef>

namespace Fw {

class MemAllocator {
  public:
    virtual ~MemAllocator() = default;

    virtual void* allocate(const FwEnumStoreType identifier,
                           FwSizeType& size,
                           bool& recoverable,
                           FwSizeType alignment = alignof(std::max_align_t)) = 0;

    virtual void deallocate(const FwEnumStoreType identifier, void* ptr) = 0;
};

}  // namespace Fw

#endif
