// Mock Zephyr kernel header for host-side testing
#ifndef ZEPHYR_KERNEL_H
#define ZEPHYR_KERNEL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* k_malloc(size_t size);
void* k_aligned_alloc(size_t alignment, size_t size);
void k_free(void* ptr);

#ifdef __cplusplus
}
#endif

#endif  // ZEPHYR_KERNEL_H
