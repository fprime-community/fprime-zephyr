# Zephyr Allocator Smoke Test

This smoke test validates the Zephyr default allocator wiring added in
`default/zephyr-config/MemoryAllocation.hpp`.

## What it checks

- `Fw::MemoryAllocation::DefaultMemoryAllocatorType` resolves to
  `Fw::ZephyrKmallocAllocator`
- `allocate` uses `k_aligned_alloc`
- requested alignment is clamped to at least `sizeof(void*)`
- failed allocation zeroes the requested `size`
- `deallocate` uses `k_free`

## Run

```bash
ci/tests/allocator/run_allocator_smoke.sh
```

The script compiles and runs a standalone C++ test binary with lightweight
stub headers for `Fw` and `zephyr` APIs.

## Integration generate/build

To run smoke test + integration build (when Zephyr SDK is installed):

```bash
export ZEPHYR_SDK_INSTALL_DIR=/path/to/zephyr-sdk
ci/tests/allocator/verify_allocator_change.sh
```

Optional environment variables:

- `PROVES_CORE_REFERENCE_DIR` (default: `../proves-core-reference`)
- `BUILD_CACHE` (default: `/tmp/proves-kmalloc-build`)
- `INSTALL_PREFIX` (default: `/tmp/proves-kmalloc-install`)
