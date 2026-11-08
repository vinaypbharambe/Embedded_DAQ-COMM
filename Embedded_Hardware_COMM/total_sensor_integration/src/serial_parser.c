#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "serial_parser.h"


/* Initialize parser */
void serial_parser_init(SerialParser *parser)
{
    if (parser == NULL)
    {
        return;
    }

    memset(parser, 0, sizeof(*parser));
}


/* Calculate XOR checksum */
unsigned char calculate_checksum(const char *data)
{
    unsigned char checksum = 0;

    if (data == NULL)
    {
        return 0;
    }

    while (*data != '\0')
    {
        checksum ^= (unsigned char)*data;
        data++;
    }

    return checksum;
}


/* Format sensor data into a serial message */
int serial_format_message(
    char *buffer,
    size_t buffer_size,
    unsigned long sequence_number,
    const SensorData *sensor
)
{
    char payload[96];

    int payload_length;
    int message_length;

    unsigned char checksum;

    if (buffer == NULL || sensor == NULL)
    {
        return -1;
    }

    /* Create message payload */
    payload_length = snprintf(
        payload,
        sizeof(payload),
        "SENSOR,%lu,%.2f,%.2f,%.2f",
        sequence_number,
        sensor->temperature,
        sensor->humidity,
        sensor->vibration
    );

    if (payload_length < 0 ||
        (size_t)payload_length >= sizeof(payload))
    {
        return -1;
    }

    /* Calculate checksum */
    checksum = calculate_checksum(payload);

    /* Construct complete serial message */
    message_length = snprintf(
        buffer,
        buffer_size,
        "$%s*%02X\n",
        payload,
        (unsigned int)checksum
    );

    if (message_length < 0 ||
        (size_t)message_length >= buffer_size)
    {
        return -1;
    }

    return message_length;
}


/* Convert hexadecimal character into integer */
static int hex_to_value(char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    character = (char)toupper((unsigned char)character);

    if (character >= 'A' && character <= 'F')
    {
        return character - 'A' + 10;
    }

    return -1;
}


/* Parse a complete serial frame */
static int parse_frame(
    const char *frame,
    ParsedSensorMessage *message
)
{
    const char *checksum_separator;

    unsigned char calculated_checksum;
    unsigned int received_checksum;

    char payload[96];

    unsigned long sequence_number;

    float temperature;
    float humidity;
    float vibration;

    int consumed = 0;

    if (frame == NULL || message == NULL)
    {
        return -1;
    }

    /* Verify start character */
    if (frame[0] != '$')
    {
        return -1;
    }

    /* Locate checksum separator */
    checksum_separator = strchr(frame, '*');

    if (checksum_separator == NULL)
    {
        return -1;
    }

    /* Ensure exactly two hexadecimal digits follow '*' */
    if (strlen(checksum_separator + 1) != 2)
    {
        return -1;
    }

    int high = hex_to_value(checksum_separator[1]);
    int low = hex_to_value(checksum_separator[2]);

    if (high < 0 || low < 0)
    {
        return -1;
    }

    received_checksum =
        (unsigned int)((high << 4) | low);

    /* Extract payload without '$' and checksum */
    size_t payload_length =
        (size_t)(checksum_separator - (frame + 1));

    if (payload_length == 0 ||
        payload_length >= sizeof(payload))
    {
        return -1;
    }

    memcpy(payload, frame + 1, payload_length);
    payload[payload_length] = '\0';

    /* Calculate checksum again */
    calculated_checksum = calculate_checksum(payload);

    if (calculated_checksum != received_checksum)
    {
        return -1;
    }

    /* Parse the payload */
    if (sscanf(
            payload,
            "SENSOR,%lu,%f,%f,%f%n",
            &sequence_number,
            &temperature,
            &humidity,
            &vibration,
            &consumed) != 4)
    {
        return -1;
    }

    /* Reject unexpected trailing characters */
    if (payload[consumed] != '\0')
    {
        return -1;
    }

    /* Reject non-finite sensor values */
    if (!isfinite(temperature) ||
        !isfinite(humidity) ||
        !isfinite(vibration))
    {
        return -1;
    }

    /* Store parsed values */
    message->sequence_number = sequence_number;

    message->sensor_data.temperature = temperature;
    message->sensor_data.humidity = humidity;
    message->sensor_data.vibration = vibration;

    /* Validate sensor measurement ranges */
    if (!validate_sensor_data(&message->sensor_data))
    {
        return -1;
    }

    return 0;
}


/* Feed received bytes into parser */
int serial_parser_feed(
    SerialParser *parser,
    char byte,
    ParsedSensorMessage *message
)
{
    if (parser == NULL || message == NULL)
    {
        return -1;
    }

    /* Detect beginning of a new message */
    if (byte == '$')
    {
        parser->length = 0;
        parser->receiving = true;
    }

    /* Ignore bytes until a start character is received */
    if (!parser->receiving)
    {
        return 0;
    }

    /* Handle message overflow */
    if (parser->length >= SERIAL_FRAME_MAX - 1)
    {
        parser->length = 0;
        parser->receiving = false;

        return -1;
    }

    /* Store received byte */
    if (byte != '\n')
    {
        parser->frame[parser->length++] = byte;
        return 0;
    }

    /* Terminate complete frame */
    parser->frame[parser->length] = '\0';

    /* Reset parser state before returning */
    parser->receiving = false;
    parser->length = 0;

    /* Parse completed frame */
    return parse_frame(parser->frame, message) == 0 ? 1 : -1;
}
