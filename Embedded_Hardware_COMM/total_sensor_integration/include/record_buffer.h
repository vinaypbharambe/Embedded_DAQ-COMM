#ifndef RECORD_BUFFER_H
#define RECORD_BUFFER_H

#include <stdbool.h>
#include <stddef.h>

#include "data_record.h"

#define RECORD_BUFFER_CAPACITY 5

typedef struct
{
    DataRecord records[RECORD_BUFFER_CAPACITY];

    size_t head;
    size_t tail;
    size_t count;

    unsigned long dropped_records;

} RecordBuffer;

void record_buffer_init(RecordBuffer *buffer);

int record_buffer_push(
    RecordBuffer *buffer,
    const DataRecord *record
);

const DataRecord *record_buffer_peek(
    const RecordBuffer *buffer
);

int record_buffer_pop(RecordBuffer *buffer);

bool record_buffer_is_empty(const RecordBuffer *buffer);

bool record_buffer_is_full(const RecordBuffer *buffer);

size_t record_buffer_size(const RecordBuffer *buffer);

#endif
