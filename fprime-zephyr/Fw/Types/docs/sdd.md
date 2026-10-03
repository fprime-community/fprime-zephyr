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
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/MemAllocator.hpp>
#include <fprime-zephyr/Fw/Types/ZephyrMultiHeapAllocator.hpp>

// Board-specific placement, e.g. with Z_GENERIC_SECTION(<section>) or addresses from the devicetree
static U8 s_fastRam[32 * 1024];
static U8 s_extRam[512 * 1024];

static Zephyr::ZephyrMultiHeapAllocator s_allocator;

void setupMemory() {
    const Zephyr::ZephyrMultiHeapAllocator::Status fastStatus =
        s_allocator.addRegion(Fw::ByteArray(s_fastRam, sizeof s_fastRam));
    FW_ASSERT(fastStatus == Zephyr::ZephyrMultiHeapAllocator::OP_OK, fastStatus);
    const Zephyr::ZephyrMultiHeapAllocator::Status extStatus =
        s_allocator.addRegion(Fw::ByteArray(s_extRam, sizeof s_extRam));
    FW_ASSERT(extStatus == Zephyr::ZephyrMultiHeapAllocator::OP_OK, extStatus);
    Fw::MemAllocatorRegistry::getInstance().registerAllocator(Fw::MemoryAllocation::MemoryAllocatorType::SYSTEM,
                                                              s_allocator);
}
```

Regions that overlap an already added region are rejected with `INVALID_REGION`.

To use a multi-heap built elsewhere (e.g. with a custom choice function), wrap it instead. `cfg` is passed through to
that heap's choice function on every allocation, and `addRegion()` returns `EXTERNAL_HEAP`. Each wrapper has its own
lock, so wrap a given `sys_multi_heap` with only one allocator:

```cpp
static Zephyr::ZephyrMultiHeapAllocator s_wrapped(boardMultiHeap, &boardCfg);
```

## ZephyrSharedMultiHeapAllocator

Each instance allocates memory of one `shared_multi_heap_attr` (cacheable, non-cacheable, external) from Zephyr's
system-wide shared multi-heap pool. Board/SoC code commonly populates the pool at boot; projects may also add regions
with the static `addRegion()`. The pool is initialized on first `addRegion()` or `allocate()` if board/SoC code has not
done so; an empty pool returns `nullptr`.

The pool holds at most `ZephyrSharedMultiHeapAllocator::MAX_REGIONS` (Zephyr's `MAX_MULTI_HEAPS`) regions in **total**
across all attributes, including regions added by board/SoC code. `addRegion()` returns `NO_MORE_REGIONS` once that many
regions have been added through it, but cannot see regions added directly by board/SoC code: projects must keep the
combined count within the limit.

```cpp
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/MemAllocator.hpp>
#include <fprime-zephyr/Fw/Types/ZephyrSharedMultiHeapAllocator.hpp>

// Board-specific placement, e.g. external RAM mapped by the board
static U8 s_extRam[512 * 1024];

static Zephyr::ZephyrSharedMultiHeapAllocator s_extAllocator(SMH_REG_ATTR_EXTERNAL);

void setupMemory() {
    const Zephyr::ZephyrSharedMultiHeapAllocator::Status status =
        Zephyr::ZephyrSharedMultiHeapAllocator::addRegion(SMH_REG_ATTR_EXTERNAL,
                                                          Fw::ByteArray(s_extRam, sizeof s_extRam));
    FW_ASSERT(status == Zephyr::ZephyrSharedMultiHeapAllocator::OP_OK, status);
    Fw::MemAllocatorRegistry::getInstance().registerAllocator(Fw::MemoryAllocation::MemoryAllocatorType::SYSTEM,
                                                              s_extAllocator);
}
```

## Concurrency

Zephyr's `sys_heap`, `sys_multi_heap`, and `shared_multi_heap` are not thread-safe. Each `ZephyrMultiHeapAllocator`
uses its own spinlock, and all `ZephyrSharedMultiHeapAllocator` instances share one spinlock. The spinlock (rather than
`Os::Mutex`) matches Zephyr's `k_heap` and allows use before `Os::init()`; it is held only for one heap operation.

The following are **not** serialized against these allocators: code calling the Zephyr APIs directly on the same heaps
(e.g. drivers using `shared_multi_heap_alloc()` concurrently with F Prime), and a second `ZephyrMultiHeapAllocator`
wrapping the same external `sys_multi_heap`. Concurrent access of this kind silently corrupts heap metadata; with
`CONFIG_ASSERT=y` it may later be caught by a `sys_heap` assertion. Such callers must share the allocator's
serialization or be confined to times when F Prime is not allocating (e.g. before or after F Prime setup).
