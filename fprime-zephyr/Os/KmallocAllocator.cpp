#include <fprime-zephyr/Os/KmallocAllocator.hpp>

#include <zephyr/kernel.h>

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
    if (size == 0U) {
        return nullptr;
    }

    alignment = normalizeAlignment(alignment);
    if (!isPowerOfTwo(alignment)) {
        return nullptr;
    }

    if (alignment <= static_cast<FwSizeType>(sizeof(void*))) {
        return k_malloc(size);
    }

    return k_aligned_alloc(alignment, size);
}

void KmallocAllocator::deallocate(const FwEnumStoreType identifier, void* ptr) {
    static_cast<void>(identifier);
    k_free(ptr);
}

}  // namespace Zephyr
