// Mock Zephyr ring buffer: same API and semantics (contiguous claims) as zephyr/sys/ring_buffer.h
#ifndef ZEPHYR_SYS_RING_BUFFER_H
#define ZEPHYR_SYS_RING_BUFFER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct ring_buf {
    uint8_t* buffer;
    uint32_t size;
    uint32_t head;        /* read index */
    uint32_t tail;        /* write index */
    uint32_t count;       /* bytes stored */
    uint32_t put_claimed; /* bytes claimed for writing, not yet finished */
    uint32_t get_claimed; /* bytes claimed for reading, not yet finished */
};
void ring_buf_init(struct ring_buf* buf, uint32_t size, uint8_t* data);
uint32_t ring_buf_capacity_get(struct ring_buf* buf);
uint32_t ring_buf_size_get(struct ring_buf* buf);
uint32_t ring_buf_space_get(struct ring_buf* buf);
int ring_buf_is_empty(struct ring_buf* buf);
uint32_t ring_buf_put_claim(struct ring_buf* buf, uint8_t** data, uint32_t size);
int ring_buf_put_finish(struct ring_buf* buf, uint32_t size);
uint32_t ring_buf_put(struct ring_buf* buf, const uint8_t* data, uint32_t size);
uint32_t ring_buf_get_claim(struct ring_buf* buf, uint8_t** data, uint32_t size);
int ring_buf_get_finish(struct ring_buf* buf, uint32_t size);
uint32_t ring_buf_get(struct ring_buf* buf, uint8_t* data, uint32_t size);
#ifdef __cplusplus
}
#endif
#endif
