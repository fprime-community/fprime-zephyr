# FprimeZephyrFatFilenameAllocator

Static pool that serves FatFs long-filename (LFN) working buffers on Zephyr, replacing Zephyr's `k_malloc`-based
`ff_memalloc`/`ff_memfree`. Deployments can then use thread-safe FatFs (`CONFIG_FS_FATFS_REENTRANT=y`) without
dynamic memory and without growing every F Prime thread stack.

## Why

FatFs needs a temporary LFN buffer inside every path-taking call (`f_open`, `f_stat`, `f_unlink`, `f_rename`,
`f_mkdir`, `f_opendir`, `f_readdir`, ...). With exFAT enabled, `f_sync` and `f_close` on a modified file also use it
to rewrite the file's directory entry. The buffer is released before the call returns. `f_read`, `f_write` and
`f_lseek` do not use it. Zephyr can place the buffer in one of three places
(`zephyr/subsys/fs/Kconfig.fatfs`):

| Mode | Buffer location | Problem for F Prime |
|---|---|---|
| `FS_FATFS_LFN_MODE_BSS` | one static buffer | not thread-safe; Kconfig forbids it with `FS_FATFS_REENTRANT` |
| `FS_FATFS_LFN_MODE_STACK` | caller's stack | 512 B to 1120 B extra on every thread that touches files |
| `FS_FATFS_LFN_MODE_HEAP` | `k_malloc` on the kernel heap | dynamic memory |

F Prime deployments call FatFs from several threads (e.g. FileUplink, FileManager, FileDownlink), so they need
`FS_FATFS_REENTRANT=y`, which rules out BSS mode. This module keeps HEAP mode but serves the buffers from a fixed,
statically allocated pool.

## Configuration

Add to the deployment's `prj.conf`:

```
CONFIG_FS_FATFS_REENTRANT=y
CONFIG_FS_FATFS_LFN_MODE_HEAP=y
CONFIG_FS_FATFS_MAX_LFN=255
CONFIG_HEAP_MEM_POOL_SIZE=256
```

- `CONFIG_FS_FATFS_MAX_LFN` sets the slot size (see below). Use the longest file name the mission needs.
- Kconfig requires `CONFIG_HEAP_MEM_POOL_SIZE > 0` for HEAP mode. FatFs does not use the kernel heap once this
  module is active, so leave it at a small value unless other Zephyr code needs it.
- `CONFIG_LTO` is not supported: link-time optimization can resolve `ff_memalloc` before `--wrap` applies. The
  module stops the CMake configuration with an error if `CONFIG_LTO` is set.

No deployment CMake or topology change is needed. On Zephyr, when `CONFIG_FS_FATFS_LFN_MODE_HEAP` is set, the module:

1. builds `FprimeZephyrFatFilenameAllocator.cpp` and links it into the Zephyr `app` target;
2. adds `-Wl,--wrap=ff_memalloc -Wl,--wrap=ff_memfree` to the Zephyr link (`zephyr_link_libraries`, so a linker without `--wrap`
   fails the link instead of silently keeping `k_malloc`).

To keep Zephyr's `k_malloc` allocator, configure with `-DFPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR=OFF`.

### Slot count

The number of slots is `FatFilenameAllocatorConfig::FPRIME_ZEPHYR_FAT_FILENAME_SLOTS` in
`default/zephyr-config/FatFilenameAllocatorCfg.hpp` (default 4). Override it like any other F Prime configuration
header.

A slot is held only for the duration of one FatFs call that needs the LFN buffer (see above). It is **not** related to
the number of open files (`CONFIG_FS_FATFS_NUM_FILES`): an open file holds no slot.

With `CONFIG_FS_FATFS_REENTRANT=y`, FatFs takes a slot only while it holds the volume mutex, so at most one slot per
mounted FAT volume is in use at a time. The count needed is the smaller of the number of mounted FAT volumes and the
number of threads that call FatFs. A single-volume deployment (one SD card) needs 1 slot; the default of 4 leaves
margin for additional volumes. The build fails if the count is below `FF_VOLUMES` (the number of FatFs disks in the
devicetree), which guarantees the pool cannot be exhausted by FatFs's own calls. Without `CONFIG_FS_FATFS_REENTRANT`, use the number of threads that may call FatFs at
the same time. An application calling `f_fdisk(..., NULL)` directly can take one more slot (see below).

When every slot is in use, the call fails with `FR_NOT_ENOUGH_CORE` (Zephyr returns `-ENOMEM`) and the exhaustion
counter is incremented.

> [!WARNING]
> With exFAT, an exhausted pool at `f_close`/`f_sync` of a modified file means the directory entry (size, first
> cluster) is not updated, so data written since the last successful sync is lost. Zephyr frees the file object
> anyway and `Os::File::close()` does not report the error. The `FF_VOLUMES` build check prevents this with
> `CONFIG_FS_FATFS_REENTRANT=y`; without it, size the pool for every thread and check that `exhaustedCount` stays
> zero.

### Slot size

Each slot serves exactly the size FatFs requests for its LFN buffer, taken from `<ff.h>`:

| FatFs configuration | Request size | At `MAX_LFN=255` |
|---|---|---|
| exFAT off | `(FF_MAX_LFN + 1) * sizeof(WCHAR)` | 512 B |
| exFAT on (`CONFIG_FS_FATFS_EXFAT=y`) | the above + `MAXDIRB(FF_MAX_LFN)` = `(FF_MAX_LFN + 44) / 15 * 32` | 1120 B |

Slots are aligned to `alignof(std::max_align_t)`, so the pool occupies about `slots * size` bytes of `.bss`
(4 * 1120 B = 4480 B for the default exFAT configuration).

## Behavior

| Call | Result |
|---|---|
| `ff_memalloc(n)`, `n` equals the slot size, a slot is free | lowest-index free slot |
| `ff_memalloc(n)`, `n` equals the slot size, no slot is free | `nullptr`; exhaustion count + 1 |
| `ff_memalloc(n)`, `n` is any other size | `nullptr`; rejected count + 1 |
| `ff_memfree(nullptr)` | no effect |
| `ff_memfree(p)`, `p` is an allocated slot | slot becomes free |
| `ff_memfree(p)`, `p` is not the start of a slot | `FW_ASSERT` |
| `ff_memfree(p)`, `p` is a slot that is already free | `FW_ASSERT` |

Pool state is protected by a Zephyr `k_spinlock`. A FatFs caller holds at most a per-volume mutex, which does not
serialize calls on different volumes. The critical section is a scan of at most `slots` flags.

### Other `ff_memalloc` callers

Only exact-size requests are served, so each thread inside FatFs holds at most one slot. The other FatFs callers of
`ff_memalloc`, with the Zephyr FatFs configuration, are:

- `dir_clear()` (from `f_mkdir` and directory growth) asks for a cluster-sized buffer (capped at 32 KiB) and halves the
  size while it is larger than one sector. These requests are rejected (counted in `rejectedCount`) and FatFs falls
  back to clearing the cluster one sector at a time from the volume window. This is slower but correct.

> [!WARNING]
> That is one single-sector write per sector of the cleared cluster (64 for 32 KiB clusters, 256 for 128 KiB), made
> with the volume mutex held, so every other FatFs call on that volume waits for the whole burst. Pre-create
> directories, or budget this latency for components that create files.
- `f_mkfs()` and `f_fdisk()` (`CONFIG_FS_FATFS_MULTI_PARTITION`) use `ff_memalloc` only when given no work buffer.
  Zephyr's `fs_mkfs` and mount-time format always pass one, and Zephyr never calls `f_fdisk`. An application calling
  `f_fdisk(..., NULL)` directly asks for `FF_MAX_SS` bytes; when that equals the slot size (exFAT off, `MAX_LFN=255`,
  512 B sectors) the request takes a pool slot for the duration of the call.

## Inspecting the pool

```cpp
#include "fprime-zephyr/Fs/FatFilenameAllocator/FprimeZephyrFatFilenameAllocator.hpp"

const Zephyr::FatFilenameAllocatorStats stats = Zephyr::FprimeZephyrFatFilenameAllocator::getStats();
```

| Field | Meaning |
|---|---|
| `slotCount` | number of slots |
| `slotBytes` | exact request size served |
| `inUse` | slots currently allocated |
| `highWaterMark` | largest `inUse` observed |
| `exhaustedCount` | correctly sized requests refused because the pool was full |
| `rejectedCount` | requests refused because of their size (includes `dir_clear()`) |

A non-zero `exhaustedCount` means the slot count is too small for the deployment.

The accessor and its module exist only when the pool is installed (Zephyr platform, `CONFIG_FS_FATFS_LFN_MODE_HEAP=y`
and `FPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR=ON`). Code that calls it must be built under the same condition, e.g. guard
it with `#if defined(CONFIG_FS_FATFS_LFN_MODE_HEAP)` and depend on `fprime-zephyr_Fs_FatFilenameAllocator` only when the
module target exists.

## Design

| File | Content |
|---|---|
| `FatFilenameAllocator.hpp` | `Zephyr::FatFilenameAllocator<SLOT_BYTES, SLOT_COUNT, LockPolicy>`: platform-independent pool, unit tested on the host |
| `FprimeZephyrFatFilenameAllocator.cpp` | sizes the pool from `<ff.h>`, provides the `k_spinlock` policy, the static instance, and the `__wrap_ff_memalloc`/`__wrap_ff_memfree` shims |
| `FprimeZephyrFatFilenameAllocator.hpp` | `getStats()` accessor |

The pool is a small template rather than a Zephyr `k_mem_slab`: a slab offers no size check or rejection counter,
detects foreign or double frees only with `__ASSERT` under `CONFIG_ASSERT`, and cannot be unit tested on the host. The
template adds exact-size matching, the counters, and `FW_ASSERT` ownership checks that are always enabled.

The module is a separate `Fs` package rather than part of `Os/`: `Os/` holds only implementations of F Prime `Os`
interfaces selected with `register_fprime_implementation`, while this module implements no F Prime interface. It
replaces a FatFs (Zephyr file-system) hook and is linked into the image whenever heap LFN mode is enabled, independent
of the `Os::File` implementation chosen.

GNU ld `--wrap=ff_memalloc` sends every call to `ff_memalloc` from another object file (FatFs's `ff.c`) to
`__wrap_ff_memalloc`. Zephyr's own definition in `zfs_ffsystem.c` is still compiled (it shares the file with the
`ff_mutex_*` functions that `FS_FATFS_REENTRANT` needs) but is no longer referenced, so section garbage collection
drops it from the image. Zephyr has no Kconfig or CMake option to leave out only its allocator.

The slot size reproduces the request computed by the private `INIT_NAMBUF`/`MAXDIRB` macros in `ff.c`, which `<ff.h>`
does not export. A `static_assert` on `FF_DEFINED` pins the FatFs revision this was checked against (R0.16, `80386`),
so a Zephyr upgrade that changes FatFs fails the build until the size is re-verified and the assertion updated.

The pool is constant-initialized static storage: there is no heap use, no `new`, and no startup constructor. A
`static_assert` on the Zephyr pool type and a `constexpr` pool in the unit test fail the build if a member stops being
constant-initializable.

### Upgrading a deployment that already uses HEAP mode

A deployment that already sets `CONFIG_FS_FATFS_LFN_MODE_HEAP=y` gets the pool when it updates fprime-zephyr, with no
configuration change. LFN buffers move from the kernel heap to `slots * size` bytes of `.bss`; at most `slots`
calls can hold one at a time; `dir_clear()` no longer receives a multi-sector heap buffer and always clears one sector
at a time; and heap space previously reserved for FatFs can be reclaimed by lowering `CONFIG_HEAP_MEM_POOL_SIZE`.
Configure with `-DFPRIME_ZEPHYR_FAT_FILENAME_ALLOCATOR=OFF` to keep the previous behavior.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| FZFA-001 | When `CONFIG_FS_FATFS_LFN_MODE_HEAP=y`, FatFs's `ff_memalloc`/`ff_memfree` calls shall resolve to the pool. | Link test, symbol inspection |
| FZFA-002 | The pool shall hold `FPRIME_ZEPHYR_FAT_FILENAME_SLOTS` statically allocated slots, each sized to FatFs's LFN request (including the exFAT directory block when exFAT is enabled) and aligned to `alignof(std::max_align_t)`. | Unit test, inspection |
| FZFA-003 | Allocation shall return the first free slot, or `nullptr` and count an exhaustion when no slot is free. | Unit test |
| FZFA-004 | Requests of any other size shall return `nullptr` and be counted as rejected. | Unit test |
| FZFA-005 | Releasing a pointer that is not an allocated slot start shall assert; releasing `nullptr` shall have no effect. | Unit test |
| FZFA-006 | The pool shall provide its slot count, slot size, in-use count, high-water mark, exhaustion count and rejected count. | Unit test |
| FZFA-007 | The pool shall use no dynamic memory and no startup constructor. | Inspection, symbol inspection |
| FZFA-008 | Concurrent allocation and release shall never hand one slot to two callers. | Unit test |

## Unit tests

`test/ut/FatFilenameAllocatorTestMain.cpp` tests the pool template on the host with a `std::mutex` lock policy and the
exFAT `MAX_LFN=255` slot size. Run from a native F Prime UT build that includes this library:

```bash
fprime-util generate --ut
fprime-util build --ut
(cd build-fprime-automatic-native-ut && ctest -R FatFilenameAllocator --output-on-failure)
```
