#include "Ring_Buffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static bool RingBuffer_IsPowerOfTwo(size_t capacity)
{
    return (capacity > 0) && ((capacity & (capacity - 1)) == 0);
}

static bool RingBuffer_IsValid(const RingBuffer_t *rb)
{
    return (rb != NULL) &&
           (rb->buffer != NULL) &&
           (rb->capacity > 0) &&
           (rb->size_element <= rb->capacity) &&
           (rb->mask == (rb->capacity - 1)) &&
           RingBuffer_IsPowerOfTwo(rb->capacity);
}

static void RingBuffer_Reset(RingBuffer_t *rb)
{
    rb->head = 0U;
    rb->tail = 0U;
    rb->size = 0U;
}

RB_Status_t RingBuffer_InitDynamic(RingBuffer_t *rb, size_t capacity, size_t size_element)
{
    if (rb == NULL)
        return RB_ERROR_NULL_POINTER;
    if (!RingBuffer_IsPowerOfTwo(capacity) || size_element == 0)
        return RB_ERROR_VALID_ARGUMENT;

    if (capacity > SIZE_MAX / size_element)
        return RB_ERROR_INSUFFICIENT_STORAGE;

    void *memory = malloc(capacity * size_element);
    if (memory == NULL)
        return RB_ERROR_ALLOCATION_FAILED;

    rb->buffer = memory;
    rb->capacity = capacity;
    rb->size_element = size_element;

    rb->mask = capacity - 1U;

    RingBuffer_Reset(rb);
    return RB_OK;
}

RB_Status_t RingBuffer_InitStatic(RingBuffer_t *rb, void *buffer, size_t capacity, size_t size_element)
{
    if (rb == NULL || buffer == NULL)
        return RB_ERROR_NULL_POINTER;

    if (!RingBuffer_IsPowerOfTwo(capacity) || size_element == 0)
        return RB_ERROR_VALID_ARGUMENT;

    if (capacity > SIZE_MAX / size_element)
        return RB_ERROR_INSUFFICIENT_STORAGE;

    size_t buffer_size = capacity * size_element;
    if (buffer_size > SIZE_MAX)
        return RB_ERROR_INSUFFICIENT_STORAGE;

    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->size_element = size_element;
    rb->mask = capacity - 1U;
    RingBuffer_Reset(rb);
    return RB_OK;
}

void RingBuffer_Destroy(RingBuffer_t *rb)
{
    if (rb == NULL)
        return;
    rb->buffer = NULL;
    rb->capacity = 0;
    rb->size_element = 0;
    rb->size = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->mask = 0;
}

bool RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    if (!RingBuffer_IsValid(rb))
        return true;
    return (rb->size == 0U);
}

bool RingBuffer_IsFull(const RingBuffer_t *rb)
{
    if (!RingBuffer_IsValid(rb))
        return false;
    return (rb->size == rb->capacity);
}

size_t RingBuffer_Size(const RingBuffer_t *rb)
{
    if (!RingBuffer_IsValid(rb))
        return 0;
    return rb->size;
}

size_t RingBuffer_Capacity(const RingBuffer_t *rb)
{
    if (!RingBuffer_IsValid(rb))
        return 0;
    return rb->capacity;
}

size_t RingBuffer_ElementSize(const RingBuffer_t *rb)
{
    if (!RingBuffer_IsValid(rb))
        return 0;
    return rb->size_element;
}

RB_Status_t RingBuffer_Push(RingBuffer_t *rb, const void *data)
{
    if (rb == NULL || data == NULL)
        return RB_ERROR_NULL_POINTER;
    if (!RingBuffer_IsValid(rb))
        return RB_ERROR_NOT_INITIALIZED;

    if (RingBuffer_IsFull(rb))
        return RB_ERROR_FULL;

    uint8_t *dst = (uint8_t *)rb->buffer + (rb->tail * rb->size_element);
    memcpy(dst, data, rb->size_element);

    rb->tail = (rb->tail + 1) & rb->mask;
    rb->size++;
    return RB_OK;
}

RB_Status_t RingBuffer_Pop(RingBuffer_t *rb, void *data)
{
    if (rb == NULL || data == NULL)
        return RB_ERROR_NULL_POINTER;
    if (!RingBuffer_IsValid(rb))
        return RB_ERROR_NOT_INITIALIZED;

    if (RingBuffer_IsEmpty(rb))
        return RB_ERROR_EMPTY;

    uint8_t *src = (uint8_t *)rb->buffer + (rb->head * rb->size_element);
    memcpy(data, src, rb->size_element);

    rb->head = (rb->head + 1) & rb->mask;
    rb->size--;
    return RB_OK;
}

void RingBuffer_Clear(RingBuffer_t *rb)
{
    if (rb == NULL || !RingBuffer_IsValid(rb))
        return;

    RingBuffer_Reset(rb);
}
