#ifndef FD_RING
#define FD_RING
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    uint8_t *buffer; // pointer to the buffer memory
    uint16_t capacity; // total capacity of the buffer (number of elements)
    volatile uint16_t head;
    volatile uint16_t tail;
}fd_ring_t;

static inline void fd_ring_init(fd_ring_t* cb,
                                      void* storage,
                                      uint16_t capacity)
{
    cb->buffer = (uint8_t*)storage;
    cb->capacity = capacity;
    cb->head = 0;
    cb->tail = 0;
}

static inline uint16_t fd_ring_size(const fd_ring_t* cb) {
    uint16_t head = cb->head; 
    uint16_t tail = cb->tail; 
    if (head >= tail)
        return head - tail;
    return cb->capacity - tail + head;
}

static inline uint16_t fd_ring_free(const fd_ring_t* cb) {
    return cb->capacity - fd_ring_size(cb) - 1;
}

static inline void fd_ring_write_commit(fd_ring_t* cb, uint16_t count) {
    uint16_t head = cb->head;
    head += count;
    if (head >= cb->capacity)
        head -= cb->capacity;
    cb->head = head;
}

static inline void fd_ring_read_commit(fd_ring_t* cb, uint16_t count) {
    uint16_t tail = cb->tail;
    tail += count;
    if (tail >= cb->capacity)
        tail -= cb->capacity;
    cb->tail = tail;
}

static inline bool fd_ring_write_reserve(fd_ring_t* cb,
                                       uint16_t count,
                                       void** writeptr1,
                                       uint16_t* cont1,
                                       void** writeptr2,
                                       uint16_t* cont2)
{
    if (count > fd_ring_free(cb))
    {
        return false;
    }

    uint16_t head = cb->head;
    uint16_t tail = cb->tail;
    uint16_t cont_space;

    if (head >= tail)
        cont_space = cb->capacity - head - (tail == 0 ? 1 : 0);
    else
        cont_space = tail - head - 1;

    *writeptr1 = cb->buffer + head;

    if (cont_space >= count) {
        *writeptr2 = NULL;
        *cont1 = count;
        *cont2 = 0;
    } else {
        *writeptr2 = cb->buffer;
        *cont1 = cont_space;
        *cont2 = count - cont_space;
    }

    return true;
}

static inline bool fd_ring_read_reserve(fd_ring_t* cb,
                                      void** readptr1,
                                      uint16_t* cont1,
                                      void** readptr2,
                                      uint16_t* cont2)
{
    uint16_t count = fd_ring_size(cb);
    
    if (count == 0)
    {
        *readptr1 = NULL;
        *cont1 = 0;
        *readptr2 = NULL;
        *cont2 = 0;
        return false;
    }

    uint16_t head = cb->head;
    uint16_t tail = cb->tail;
    uint16_t cont_space;
    
    if (tail >= head)
        cont_space = cb->capacity - tail;
    else
        cont_space = head - tail;

    *readptr1 = cb->buffer + tail;

    if (cont_space >= count) {
        *readptr2 = NULL;
        *cont1 = count;
        *cont2 = 0;
    } else {
        *readptr2 = cb->buffer;
        *cont1 = cont_space;
        *cont2 = count - cont_space;
    }

    return true;
}

#ifdef __cplusplus
}
#endif

#endif
