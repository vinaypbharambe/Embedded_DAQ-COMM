#ifndef SERIAL_PARSER_H
#define SERIAL_PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "sensor.h"

#define SERIAL_FRAME_MAX 128

/* Parsed serial message */
typedef struct
{
    unsigned long sequence_number;
    SensorData sensor_data;

} ParsedSensorMessage;


/* Serial parser state */
typedef struct
{
    char frame[SERIAL_FRAME_MAX];

    size_t length;

    bool receiving;

} SerialParser;


/* Initialize parser */
void serial_parser_init(SerialParser *parser);


/* Calculate XOR checksum */
unsigned char calculate_checksum(const char *data);


/* Format sensor data into a serial message */
int serial_format_message(
    char *buffer,
    size_t buffer_size,
    unsigned long sequence_number,
    const SensorData *sensor
);


/*
 * Feed received bytes into the parser.
 *
 * Returns:
 *  1 = valid message received
 *  0 = message incomplete
 * -1 = invalid or corrupted message
 */
int serial_parser_feed(
    SerialParser *parser,
    char byte,
    ParsedSensorMessage *message
);

#endif
