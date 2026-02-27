// Host-side smoke tests for ZephyrKmallocAllocator
//
// Mocks Zephyr kernel functions and fprime base types to verify allocator
// logic without needing a real Zephyr environment.

#include <cassert>
#include <cstdio>
#include <cstdlib>

// --- Mock state ---
enum CallType { CALL_NONE, CALL_KMALLOC, CALL_KALIGNED };

static CallType g_lastCall = CALL_NONE;
static size_t g_lastSize = 0;
static size_t g_lastAlignment = 0;
static void* g_returnPtr = nullptr;
static void* g_freedPtr = nullptr;

// --- Mock Zephyr kernel functions ---
extern "C" {

void* k_malloc(size_t size) {
    g_lastCall = CALL_KMALLOC;
    g_lastSize = size;
    g_lastAlignment = 0;
    return g_returnPtr;
}

void* k_aligned_alloc(size_t alignment, size_t size) {
    g_lastCall = CALL_KALIGNED;
    g_lastSize = size;
    g_lastAlignment = alignment;
    return g_returnPtr;
}

void k_free(void* ptr) {
    g_freedPtr = ptr;
}

}  // extern "C"

// Include the real allocator header under test.
// Mock headers are found via -Imocks, real header via -I path.
#include "MemoryAllocation.hpp"

static void reset() {
    g_lastCall = CALL_NONE;
    g_lastSize = 0;
    g_lastAlignment = 0;
    g_returnPtr = nullptr;
    g_freedPtr = nullptr;
}

static char fake_mem[256];

int main() {
    Fw::ZephyrKmallocAllocator alloc;
    FwSizeType size;
    bool recoverable;
    void* result;

    // Test 1: Default alignment uses k_malloc
    printf("Test 1: Default alignment -> k_malloc... ");
    reset();
    g_returnPtr = fake_mem;
    size = 64;
    result = alloc.allocate(0, size, recoverable);
    assert(g_lastCall == CALL_KMALLOC);
    assert(g_lastSize == 64);
    assert(result == fake_mem);
    assert(size == 64);
    assert(!recoverable);
    printf("PASS\n");

    // Test 2: Small alignment (== sizeof(void*)) uses k_malloc
    printf("Test 2: Small alignment -> k_malloc... ");
    reset();
    g_returnPtr = fake_mem;
    size = 128;
    result = alloc.allocate(0, size, recoverable, sizeof(void*));
    assert(g_lastCall == CALL_KMALLOC);
    assert(g_lastSize == 128);
    printf("PASS\n");

    // Test 3: Alignment of 1 uses k_malloc
    printf("Test 3: Alignment=1 -> k_malloc... ");
    reset();
    g_returnPtr = fake_mem;
    size = 32;
    result = alloc.allocate(0, size, recoverable, 1);
    assert(g_lastCall == CALL_KMALLOC);
    printf("PASS\n");

    // Test 4: Large power-of-2 alignment uses k_aligned_alloc
    printf("Test 4: Large power-of-2 alignment -> k_aligned_alloc... ");
    reset();
    g_returnPtr = fake_mem;
    size = 256;
    result = alloc.allocate(0, size, recoverable, 64);
    assert(g_lastCall == CALL_KALIGNED);
    assert(g_lastSize == 256);
    assert(g_lastAlignment == 64);
    assert(result == fake_mem);
    printf("PASS\n");

    // Test 5: Non-power-of-2 alignment is rounded up to next power of 2
    printf("Test 5: Non-power-of-2 alignment rounded up... ");
    reset();
    g_returnPtr = fake_mem;
    size = 100;
    // 48 is not a power of 2 -> should round up to 64
    result = alloc.allocate(0, size, recoverable, 48);
    assert(g_lastCall == CALL_KALIGNED);
    assert(g_lastAlignment == 64);
    assert(g_lastSize == 100);
    printf("PASS\n");

    // Test 6: Another non-power-of-2: 12 -> 16
    printf("Test 6: Alignment 12 -> rounds to 16... ");
    reset();
    g_returnPtr = fake_mem;
    size = 50;
    result = alloc.allocate(0, size, recoverable, 12);
    assert(g_lastCall == CALL_KALIGNED);
    assert(g_lastAlignment == 16);
    printf("PASS\n");

    // Test 7: Zero alignment treated as 1, uses k_malloc
    printf("Test 7: Zero alignment -> k_malloc... ");
    reset();
    g_returnPtr = fake_mem;
    size = 32;
    result = alloc.allocate(0, size, recoverable, 0);
    assert(g_lastCall == CALL_KMALLOC);
    printf("PASS\n");

    // Test 8: Allocation failure returns nullptr and zeroes size
    printf("Test 8: Allocation failure -> nullptr, size=0... ");
    reset();
    g_returnPtr = nullptr;
    size = 64;
    result = alloc.allocate(0, size, recoverable);
    assert(result == nullptr);
    assert(size == 0);
    printf("PASS\n");

    // Test 9: Deallocate calls k_free with correct pointer
    printf("Test 9: Deallocate -> k_free... ");
    reset();
    alloc.deallocate(0, fake_mem);
    assert(g_freedPtr == fake_mem);
    printf("PASS\n");

    // Test 10: DefaultMemoryAllocatorType is ZephyrKmallocAllocator
    printf("Test 10: DefaultMemoryAllocatorType works... ");
    Fw::MemoryAllocation::DefaultMemoryAllocatorType defaultAlloc;
    reset();
    g_returnPtr = fake_mem;
    size = 10;
    result = defaultAlloc.allocate(0, size, recoverable);
    assert(g_lastCall == CALL_KMALLOC);
    assert(result == fake_mem);
    printf("PASS\n");

    printf("\nAll %d tests passed!\n", 10);
    return 0;
}
