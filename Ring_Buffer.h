#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    RB_OK = 0,
    RB_ERROR_NULL_POINTER,
    RB_ERROR_VALID_ARGUMENT,
    RB_ERROR_ALLOCATION_FAILED,
    RB_ERROR_NOT_INITIALIZED,
    RB_ERROR_FULL,
    RB_ERROR_EMPTY,
    RB_ERROR_INSUFFICIENT_STORAGE
} RB_Status_t;

typedef struct
{
    void *buffer;
    size_t capacity;
    size_t size_element;
    size_t size;
    size_t head;
    size_t tail;
    size_t mask;
} RingBuffer_t;

RB_Status_t RingBuffer_InitDynamic(RingBuffer_t *rb, size_t capacity, size_t size_element);
RB_Status_t RingBuffer_InitStatic(RingBuffer_t *rb, void *buffer, size_t capacity, size_t size_element);

void RingBuffer_Destroy(RingBuffer_t *rb);

bool RingBuffer_IsFull(const RingBuffer_t *rb);
bool RingBuffer_IsEmpty(const RingBuffer_t *rb);

size_t RingBuffer_Size(const RingBuffer_t *rb);
size_t RingBuffer_Capacity(const RingBuffer_t *rb);
size_t RingBuffer_ElementSize(const RingBuffer_t *rb);

RB_Status_t RingBuffer_Push(RingBuffer_t *rb, const void *data);
RB_Status_t RingBuffer_Pop(RingBuffer_t *rb, void *data);

void RingBuffer_Clear(RingBuffer_t *rb);
#endif // RINGBUFFER_H
