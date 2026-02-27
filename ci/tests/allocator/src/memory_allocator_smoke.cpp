#include "default/zephyr-config/MemoryAllocation.hpp"

#include <cassert>
#include <cstdlib>
#include <type_traits>

namespace {

std::size_t g_last_alignment = 0;
std::size_t g_last_size = 0;
bool g_force_alloc_fail = false;
bool g_free_called = false;

}

extern "C" void* k_aligned_alloc(std::size_t alignment, std::size_t size) {
    g_last_alignment = alignment;
    g_last_size = size;
    if (g_force_alloc_fail) {
        return nullptr;
    }
    const std::size_t alloc_size = (size == 0U) ? 1U : size;
    return std::malloc(alloc_size);
}

extern "C" void k_free(void* ptr) {
    g_free_called = true;
    std::free(ptr);
}

int main() {
    static_assert(std::is_same<Fw::MemoryAllocation::DefaultMemoryAllocatorType,
                               Fw::ZephyrKmallocAllocator>::value,
                  "Default allocator must be ZephyrKmallocAllocator");

    Fw::MemoryAllocation::DefaultMemoryAllocatorType allocator;

    // Small alignment is rounded up to sizeof(void*)
    {
        FwSizeType size = 32U;
        bool recoverable = true;
        void* ptr = allocator.allocate(7U, size, recoverable, 1U);
        assert(ptr != nullptr);
        assert(size == 32U);
        assert(recoverable == false);
        assert(g_last_alignment == sizeof(void*));
        assert(g_last_size == 32U);
        g_free_called = false;
        allocator.deallocate(7U, ptr);
        assert(g_free_called);
    }

    // Valid power-of-two alignment is passed through
    {
        FwSizeType size = 64U;
        bool recoverable = true;
        void* ptr = allocator.allocate(11U, size, recoverable, 64U);
        assert(ptr != nullptr);
        assert(size == 64U);
        assert(recoverable == false);
        assert(g_last_alignment == 64U);
        assert(g_last_size == 64U);
        allocator.deallocate(11U, ptr);
    }

    // Non-power-of-two alignment is rounded up
    {
        FwSizeType size = 48U;
        bool recoverable = true;
        void* ptr = allocator.allocate(9U, size, recoverable, 12U);
        assert(ptr != nullptr);
        assert(size == 48U);
        assert(recoverable == false);
        assert(g_last_alignment == 16U);
        assert(g_last_size == 48U);
        allocator.deallocate(9U, ptr);
    }

    // Size is rounded up to a multiple of alignment
    {
        FwSizeType size = 55U;
        bool recoverable = true;
        void* ptr = allocator.allocate(14U, size, recoverable, 16U);
        assert(ptr != nullptr);
        assert(size == 55U);
        assert(recoverable == false);
        assert(g_last_alignment == 16U);
        assert(g_last_size == 64U);
        allocator.deallocate(14U, ptr);
    }

    // Allocation failure returns nullptr and zeroes size
    {
        g_force_alloc_fail = true;
        FwSizeType size = 55U;
        bool recoverable = true;
        void* ptr = allocator.allocate(13U, size, recoverable, 16U);
        assert(ptr == nullptr);
        assert(size == 0U);
        assert(recoverable == false);
        g_force_alloc_fail = false;
    }

    return 0;
}
