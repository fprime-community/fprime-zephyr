// Mock Zephyr device
#ifndef ZEPHYR_DEVICE_H
#define ZEPHYR_DEVICE_H
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
struct device { const char* name; };
bool device_is_ready(const struct device* dev);
#ifdef __cplusplus
}
#endif
#endif
