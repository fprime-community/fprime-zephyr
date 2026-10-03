#include <fprime-zephyr/Os/KmallocAllocator.hpp>

#include <zephyr/kernel.h>

#include <limits>

namespace Zephyr {

bool KmallocAllocator::isPowerOfTwo(const FwSizeType value) {
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

FwSizeType KmallocAllocator::normalizeAlignment(FwSizeType alignment) {
    const auto pointerAlignment = static_cast<FwSizeType>(sizeof(void*));
    if (alignment < pointerAlignment) {
        return pointerAlignment;
    }
    return alignment;
}

void* KmallocAllocator::allocate(const FwEnumStoreType identifier,
                                 FwSizeType& size,
                                 bool& recoverable,
                                 FwSizeType alignment) {
    static_cast<void>(identifier);

    recoverable = false;
    void* memory = nullptr;
    alignment = normalizeAlignment(alignment);
    // Zephyr takes size_t: a larger FwSizeType request would be truncated into a smaller allocation
    const auto maxRequest = static_cast<FwSizeType>(std::numeric_limits<size_t>::max());
    if ((size != 0U) && (size <= maxRequest) && isPowerOfTwo(alignment) && (alignment <= maxRequest)) {
        if (alignment <= static_cast<FwSizeType>(sizeof(void*))) {
            memory = k_malloc(static_cast<size_t>(size));
        } else {
            memory = k_aligned_alloc(static_cast<size_t>(alignment), static_cast<size_t>(size));
        }
    }
    if (memory == nullptr) {
        size = 0;
    }
    return memory;
}

void KmallocAllocator::deallocate(const FwEnumStoreType identifier, void* ptr) {
    static_cast<void>(identifier);
    k_free(ptr);
}

}  // namespace Zephyr
