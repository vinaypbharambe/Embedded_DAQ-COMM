#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

#include "nmea_parser.h"


/* Calculate NMEA XOR checksum */
static unsigned char calculate_nmea_checksum(const char *data)
{
    unsigned char checksum = 0;

    while (*data != '\0')
    {
        checksum ^= (unsigned char)*data;
        data++;
    }

    return checksum;
}


/* Convert hexadecimal character to integer */
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


/* Convert a string into a finite double */
static int parse_double_value(const char *text, double *value)
{
    char *end_pointer;

    if (text == NULL || value == NULL || *text == '\0')
    {
        return -1;
    }

    errno = 0;

    *value = strtod(text, &end_pointer);

    if (errno != 0 ||
        end_pointer == text ||
        *end_pointer != '\0' ||
        !isfinite(*value))
    {
        return -1;
    }

    return 0;
}


/* Convert a string into an integer */
static int parse_integer_value(const char *text, int *value)
{
    char *end_pointer;

    if (text == NULL || value == NULL || *text == '\0')
    {
        return -1;
    }

    errno = 0;

    long result = strtol(text, &end_pointer, 10);

    if (errno != 0 ||
        end_pointer == text ||
        *end_pointer != '\0' ||
        result < 0 ||
        result > 1000000)
    {
        return -1;
    }

    *value = (int)result;

    return 0;
}


/* Convert NMEA degrees and minutes into decimal degrees */
static int convert_coordinates(
    const char *coordinate,
    char direction,
    bool is_latitude,
    double *decimal_degrees
)
{
    double raw_value;
    double degrees;
    double minutes;

    if (coordinate == NULL || decimal_degrees == NULL)
    {
        return -1;
    }

    if (direction != 'N' && direction != 'S' &&
        direction != 'E' && direction != 'W')
    {
        return -1;
    }

    if (is_latitude &&
        direction != 'N' && direction != 'S')
    {
        return -1;
    }

    if (!is_latitude &&
        direction != 'E' && direction != 'W')
    {
        return -1;
    }

    if (parse_double_value(coordinate, &raw_value) != 0 ||
        raw_value < 0.0)
    {
        return -1;
    }

    degrees = (double)((int)(raw_value / 100.0));

    minutes = raw_value - (degrees * 100.0);

    if (minutes < 0.0 || minutes >= 60.0)
    {
        return -1;
    }

    if (degrees > (is_latitude ? 90.0 : 180.0))
    {
        return -1;
    }

    if (degrees == (is_latitude ? 90.0 : 180.0) &&
        minutes != 0.0)
    {
        return -1;
    }

    *decimal_degrees = degrees + (minutes / 60.0);

    if (direction == 'S' || direction == 'W')
    {
        *decimal_degrees = -*decimal_degrees;
    }

    return 0;
}


/* Split a comma-separated payload while preserving empty fields */
static int split_fields(
    char *payload,
    char *fields[],
    size_t maximum_fields
)
{
    size_t count = 0;

    char *start = payload;

    for (char *current = payload; ; current++)
    {
        if (*current == ',' || *current == '\0')
        {
            if (count >= maximum_fields)
            {
                return -1;
            }

            fields[count++] = start;

            if (*current == '\0')
            {
                break;
            }

            *current = '\0';
            start = current + 1;
        }
    }

    return (int)count;
}


/* Parse complete NMEA GPGGA sentence */
int nmea_parse_gpgga(
    const char *sentence,
    GPSData *gps
)
{
    char buffer[NMEA_MAX_LENGTH];

    char *checksum_separator;

    char *fields[16];

    size_t sentence_length;

    unsigned int received_checksum;

    unsigned char calculated_checksum;

    int field_count;

    double temporary_value;

    if (sentence == NULL || gps == NULL)
    {
        return -1;
    }

    sentence_length = strlen(sentence);

    if (sentence_length == 0 ||
        sentence_length >= sizeof(buffer))
    {
        return -1;
    }

    memcpy(buffer, sentence, sentence_length + 1);

    /* Remove trailing CR/LF characters */
    while (sentence_length > 0 &&
           (buffer[sentence_length - 1] == '\r' ||
            buffer[sentence_length - 1] == '\n'))
    {
        buffer[--sentence_length] = '\0';
    }

    /* Verify start character */
    if (buffer[0] != '$')
    {
        return -1;
    }

    /* Locate checksum separator */
    checksum_separator = strchr(buffer, '*');

    if (checksum_separator == NULL ||
        strlen(checksum_separator + 1) != 2)
    {
        return -1;
    }

    /* Parse checksum digits */
    int high = hex_to_value(checksum_separator[1]);
    int low = hex_to_value(checksum_separator[2]);

    if (high < 0 || low < 0)
    {
        return -1;
    }

    received_checksum =
        (unsigned int)((high << 4) | low);

    /* Calculate checksum over characters between $ and * */
    *checksum_separator = '\0';

    calculated_checksum =
        calculate_nmea_checksum(buffer + 1);

    if (calculated_checksum != received_checksum)
    {
        return -1;
    }

    /* Check NMEA sentence identifier */
    if (strncmp(buffer + 1, "GPGGA,", 6) != 0)
    {
        return -1;
    }

    /* Extract payload */
    field_count = split_fields(
        buffer + 1,
        fields,
        sizeof(fields) / sizeof(fields[0])
    );

    if (field_count != 15 ||
        strcmp(fields[0], "GPGGA") != 0)
    {
        return -1;
    }

    /* Validate UTC time */
    if (strlen(fields[1]) < 6 ||
        strlen(fields[1]) > 10)
    {
        return -1;
    }

    for (size_t i = 0; fields[1][i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)fields[1][i]) &&
            fields[1][i] != '.')
        {
            return -1;
        }
    }

    if (fields[1][2] > '5' ||
        fields[1][4] > '5')
    {
        return -1;
    }

    if (atoi(fields[1]) / 10000 > 23)
    {
        return -1;
    }

    if (snprintf(
            gps->utc_time,
            sizeof(gps->utc_time),
            "%s",
            fields[1]
        ) >= (int)sizeof(gps->utc_time))
    {
        return -1;
    }

    /* Extract latitude */
    if (convert_coordinates(
            fields[2],
            fields[3][0],
            true,
            &gps->latitude
        ) != 0 ||
        fields[3][1] != '\0')
    {
        return -1;
    }

    gps->latitude_direction = fields[3][0];

    /* Extract longitude */
    if (convert_coordinates(
            fields[4],
            fields[5][0],
            false,
            &gps->longitude
        ) != 0 ||
        fields[5][1] != '\0')
    {
        return -1;
    }

    gps->longitude_direction = fields[5][0];

    /* Extract fix quality and satellite count */
    if (parse_integer_value(
            fields[6],
            &gps->fix_quality
        ) != 0 ||
        parse_integer_value(
            fields[7],
            &gps->satellites
        ) != 0)
    {
        return -1;
    }

    if (gps->fix_quality > 8 ||
        gps->satellites > 99)
    {
        return -1;
    }

    /* Extract HDOP */
    if (parse_double_value(
            fields[8],
            &temporary_value
        ) != 0 ||
        temporary_value < 0.0)
    {
        return -1;
    }

    gps->hdop = (float)temporary_value;

    /* Extract altitude */
    if (parse_double_value(
            fields[9],
            &temporary_value
        ) != 0)
    {
        return -1;
    }

    gps->altitude = (float)temporary_value;

    /* Extract geoid separation */
    if (parse_double_value(
            fields[11],
            &temporary_value
        ) != 0)
    {
        return -1;
    }

    gps->geoid_separation = (float)temporary_value;

    /* Determine whether GPS position fix is available */
    gps->has_fix = (gps->fix_quality > 0);

    return 0;
}