# Zephyr Multi-Heap Memory Allocators

`fprime-zephyr/Fw/Types` provides two `Fw::MemAllocator` implementations that let F Prime draw memory from several
discontiguous RAM regions (e.g. internal SRAM, DTCM, external PSRAM/SDRAM) through Zephyr's multi-heap APIs.

| Class                                    | Zephyr API          | Kconfig                    |
|------------------------------------------|---------------------|----------------------------|
| `Zephyr::ZephyrMultiHeapAllocator`       | `sys_multi_heap`    | `CONFIG_MULTI_HEAP=y`        |
| `Zephyr::ZephyrSharedMultiHeapAllocator` | `shared_multi_heap` | `CONFIG_SHARED_MULTI_HEAP=y` |

Both allocators:

- ignore the allocation identifier and always report memory as not recoverable
- honor any power-of-two alignment (asserting otherwise)
- set `size` to `0` and return `nullptr` when the request cannot be satisfied
- serialize their own calls with a `k_spinlock`

Add the module as a dependency of the deployment (or component) that uses it:

```cmake
register_fprime_zephyr_deployment(
    ...
    DEPENDS
        fprime-zephyr_Fw_Types
)
```

## ZephyrMultiHeapAllocator

A default-constructed allocator owns its `sys_multi_heap` and a first-fit choice function. Up to
`ZephyrMultiHeapAllocator::MAX_REGIONS` (Zephyr's `MAX_MULTI_HEAPS`) regions of at least
`ZephyrMultiHeapAllocator::MIN_REGION_SIZE` bytes are added with `addRegion()`; allocations are tried in the order the
regions were added. Regions must remain valid for the lifetime of the allocator.

```cpp
#include <fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp>

// Board-specific placement, e.g. with Z_GENERIC_SECTION(<section>) or addresses from the devicetree
static U8 s_fastRam[32 * 1024];
static U8 s_extRam[512 * 1024];

static Zephyr::ZephyrMultiHeapAllocator s_allocator;

void setupMemory() {
    FW_ASSERT(s_allocator.addRegion(Fw::ByteArray(s_fastRam, sizeof s_fastRam)) ==
              Zephyr::ZephyrMultiHeapAllocator::OP_OK);
    FW_ASSERT(s_allocator.addRegion(Fw::ByteArray(s_extRam, sizeof s_extRam)) ==
              Zephyr::ZephyrMultiHeapAllocator::OP_OK);
    Fw::MemAllocatorRegistry::getInstance().registerAllocator(MemoryAllocation::MemoryAllocatorType::SYSTEM,
                                                              s_allocator);
}
```

To use a multi-heap built elsewhere (e.g. with a custom choice function), wrap it instead. `cfg` is passed through to
that heap's choice function on every allocation, and `addRegion()` returns `EXTERNAL_HEAP`:

```cpp
static Zephyr::ZephyrMultiHeapAllocator s_wrapped(boardMultiHeap, &boardCfg);
```

## ZephyrSharedMultiHeapAllocator

Each instance allocates memory of one `shared_multi_heap_attr` (cacheable, non-cacheable, external) from Zephyr's
system-wide shared multi-heap pool. Board/SoC code commonly populates the pool at boot; projects may also add regions
with the static `addRegion()`, which initializes the pool on first use. Up to `MAX_MULTI_HEAPS` regions may be added per
attribute.

```cpp
#include <fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp>

static Zephyr::ZephyrSharedMultiHeapAllocator s_extAllocator(SMH_REG_ATTR_EXTERNAL);

void setupMemory() {
    FW_ASSERT(Zephyr::ZephyrSharedMultiHeapAllocator::addRegion(SMH_REG_ATTR_EXTERNAL,
                                                                Fw::ByteArray(s_extRam, sizeof s_extRam)) ==
              Zephyr::ZephyrSharedMultiHeapAllocator::OP_OK);
    Fw::MemAllocatorRegistry::getInstance().registerAllocator(MemoryAllocation::MemoryAllocatorType::SYSTEM,
                                                              s_extAllocator);
}
```

## Concurrency

Zephyr's `sys_heap`, `sys_multi_heap`, and `shared_multi_heap` are not thread-safe. Each `ZephyrMultiHeapAllocator`
uses its own spinlock, and all `ZephyrSharedMultiHeapAllocator` instances share one spinlock. Code calling the Zephyr
APIs directly on the same heaps is not serialized against these allocators.
