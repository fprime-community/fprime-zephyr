#ifndef ZEPHYR_KERNEL_H
#define ZEPHYR_KERNEL_H

#include <cstddef>

extern "C" {
void* k_aligned_alloc(std::size_t alignment, std::size_t size);
void k_free(void* ptr);
}

#endif
