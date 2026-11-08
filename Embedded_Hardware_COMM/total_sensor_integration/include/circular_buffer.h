#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stddef.h>

#include "data_record.h"

#define BUFFER_CAPACITY 5

typedef struct
{
    DataRecord records[BUFFER_CAPACITY];

    size_t head;
    size_t tail;
    size_t count;

    unsigned long dropped_records;

} CircularBuffer;

/*
 * Initialize the circular buffer.
 */
void circular_buffer_init(CircularBuffer *buffer);

/*
 * Insert a record.
 *
 * Returns:
 *  0 = Successfully inserted
 *  1 = Buffer was full; oldest record dropped
 * -1 = Invalid argument
 */
int circular_buffer_push(
    CircularBuffer *buffer,
    const DataRecord *record
);

/*
 * Remove the oldest record.
 *
 * Returns:
 *  0 = Successfully removed
 * -1 = Buffer empty or invalid argument
 */
int circular_buffer_pop(
    CircularBuffer *buffer,
    DataRecord *record
);

/*
 * Return the number of stored records.
 */
size_t circular_buffer_count(const CircularBuffer *buffer);

/*
 * Check whether the buffer is empty.
 */
int circular_buffer_is_empty(const CircularBuffer *buffer);

/*
 * Check whether the buffer is full.
 */
int circular_buffer_is_full(const CircularBuffer *buffer);

#endif
