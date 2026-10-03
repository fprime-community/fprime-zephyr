# Zephyr Shared Multi-Heap Memory Allocator

`Zephyr::ZephyrSharedMultiHeapAllocator` is an `Fw::MemAllocator` backed by Zephyr's system-wide `shared_multi_heap`
pool (`CONFIG_SHARED_MULTI_HEAP=y`). The pool groups RAM regions by attribute (`SMH_REG_ATTR_CACHEABLE`,
`SMH_REG_ATTR_NON_CACHEABLE`, `SMH_REG_ATTR_EXTERNAL`) and is shared with Zephyr drivers. F Prime can therefore draw its
setup-time memory from regions such as external PSRAM while that memory stays available to the rest of the system.

Each instance allocates memory of one attribute. Every instance:

- ignores the allocation identifier and always reports memory as not recoverable
- honors any power-of-two alignment (asserting otherwise)
- sets `size` to `0` and returns `nullptr` when the request cannot be satisfied

Add the module as a dependency of the deployment (or component) that uses it:

```cmake
register_fprime_zephyr_deployment(
    ...
    DEPENDS
        fprime-zephyr_Fw_Types
)
```

## Populating the Pool

Board/SoC code commonly populates the pool at boot. For example, Zephyr's ESP32 PSRAM support and the STM32 OSPI/XSPI
PSRAM drivers add PSRAM as `SMH_REG_ATTR_EXTERNAL`. On such boards, construct the allocator and register it; no
`addRegion()` call is needed.

Otherwise the project adds regions with the static `addRegion()` once the memory is accessible, e.g. after its PSRAM
controller is configured. Do not add a region that board/SoC code has already added: `addRegion()` cannot see such
regions, so the overlap would go undetected. The pool is initialized by the first `addRegion()` or `allocate()` if
board/SoC code has not done so; allocating from an empty pool returns `nullptr`.

```cpp
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/MemAllocator.hpp>
#include <fprime-zephyr/Fw/Types/ZephyrSharedMultiHeapAllocator.hpp>

static Zephyr::ZephyrSharedMultiHeapAllocator s_psramAllocator(SMH_REG_ATTR_EXTERNAL);

// psram/psramSize: PSRAM mapped by the board's memory controller
void setupMemory(U8* const psram, const FwSizeType psramSize) {
    const Zephyr::ZephyrSharedMultiHeapAllocator::Status status =
        Zephyr::ZephyrSharedMultiHeapAllocator::addRegion(SMH_REG_ATTR_EXTERNAL, Fw::ByteArray(psram, psramSize));
    FW_ASSERT(status == Zephyr::ZephyrSharedMultiHeapAllocator::OP_OK, status);
    Fw::MemAllocatorRegistry::getInstance().registerAllocator(Fw::MemoryAllocation::MemoryAllocatorType::SYSTEM,
                                                              s_psramAllocator);
}
```

## Limits

- The pool holds at most `ZephyrSharedMultiHeapAllocator::MAX_REGIONS` (Zephyr's `MAX_MULTI_HEAPS`) regions in **total**
  across all attributes, including regions added by board/SoC code. `addRegion()` returns `NO_MORE_REGIONS` once that
  many regions have been added through it, but cannot count board/SoC regions: projects must keep the combined count
  within the limit.
- Regions must be at least `ZephyrSharedMultiHeapAllocator::MIN_REGION_SIZE` bytes and remain valid for the lifetime of
  the program. Regions overlapping one already added through `addRegion()` are rejected with `INVALID_REGION`.

> [!WARNING]
> `sys_heap` bounds its free-list search with `CONFIG_SYS_HEAP_ALLOC_LOOPS` (default 3) to keep allocation time
> constant. In a fragmented region a request can therefore fail even though a fitting block exists, and callers using
> `Fw::MemAllocator::checkedAllocate()` will assert. Perform assert-on-failure setup allocations before heap churn and
> keep contiguous headroom in each region. Raising `CONFIG_SYS_HEAP_ALLOC_LOOPS` reduces such failures at the cost of a
> longer search, repeated per region, while the allocator's spinlock (interrupts masked) is held.

## Concurrency

Zephyr's `shared_multi_heap` performs no locking of its own. All `ZephyrSharedMultiHeapAllocator` instances share one
spinlock. The spinlock (rather than `Os::Mutex`, a `k_mutex`) matches Zephyr's `k_heap` and allows use from ISRs. It
masks interrupts for one allocator call; a failing `allocate()` searches every region of its attribute in turn (up to
`MAX_REGIONS`), each search bounded by `CONFIG_SYS_HEAP_ALLOC_LOOPS`, so budget interrupt latency for that full walk.

> [!IMPORTANT]
> Zephyr code calling `shared_multi_heap_*` directly (e.g. display or video drivers) is **not** serialized with these
> allocators. Concurrent access silently corrupts heap metadata; with `CONFIG_ASSERT=y` it may later be caught by a
> `sys_heap` assertion. Drivers that allocate during boot (before `main()`) are safe. Drivers that allocate at run time
> must not do so while F Prime is allocating, e.g. defer them until F Prime setup has completed.
