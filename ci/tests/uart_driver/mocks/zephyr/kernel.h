// Mock Zephyr kernel: semaphore with a test hook on take
#ifndef ZEPHYR_KERNEL_H
#define ZEPHYR_KERNEL_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int64_t k_ticks_t;
typedef struct { k_ticks_t ticks; } k_timeout_t;
#define K_MSEC(ms) ((k_timeout_t){ (k_ticks_t)(ms) })
struct k_sem { unsigned int count; unsigned int limit; };
int k_sem_init(struct k_sem* sem, unsigned int initial, unsigned int limit);
void k_sem_give(struct k_sem* sem);
int k_sem_take(struct k_sem* sem, k_timeout_t timeout);
#ifdef __cplusplus
}
#endif
#endif
