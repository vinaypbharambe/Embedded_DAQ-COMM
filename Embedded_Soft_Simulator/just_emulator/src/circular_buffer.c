#include <string.h>

#include "circular_buffer.h"

void circular_buffer_init(CircularBuffer *buffer)
{
    if (buffer == NULL)
    {
        return;
    }

    memset(buffer, 0, sizeof(*buffer));
}

int circular_buffer_push(
    CircularBuffer *buffer,
    const DataRecord *record
)
{
    int result = 0;

    if (buffer == NULL || record == NULL)
    {
        return -1;
    }

    /*
     * If full, discard the oldest record.
     */
    if (buffer->count == BUFFER_CAPACITY)
    {
        buffer->head =
            (buffer->head + 1) % BUFFER_CAPACITY;

        buffer->count--;

        buffer->dropped_records++;

        result = 1;
    }

    /*
     * Insert the new record at the tail.
     */
    buffer->records[buffer->tail] = *record;

    buffer->tail =
        (buffer->tail + 1) % BUFFER_CAPACITY;

    buffer->count++;

    return result;
}

int circular_buffer_pop(
    CircularBuffer *buffer,
    DataRecord *record
)
{
    if (buffer == NULL || record == NULL)
    {
        return -1;
    }

    if (buffer->count == 0)
    {
        return -1;
    }

    /*
     * Retrieve the oldest record.
     */
    *record = buffer->records[buffer->head];

    /*
     * Advance head to the next record.
     */
    buffer->head =
        (buffer->head + 1) % BUFFER_CAPACITY;

    buffer->count--;

    return 0;
}

size_t circular_buffer_count(const CircularBuffer *buffer)
{
    if (buffer == NULL)
    {
        return 0;
    }

    return buffer->count;
}

int circular_buffer_is_empty(const CircularBuffer *buffer)
{
    return buffer == NULL || buffer->count == 0;
}

int circular_buffer_is_full(const CircularBuffer *buffer)
{
    return buffer != NULL &&
           buffer->count == BUFFER_CAPACITY;
}
