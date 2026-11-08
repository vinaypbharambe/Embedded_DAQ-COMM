#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "gps.h"


/* Calculate NMEA XOR checksum */
static unsigned char gps_checksum(const char *data)
{
    unsigned char checksum = 0;

    while (*data != '\0')
    {
        checksum ^= (unsigned char)*data;
        data++;
    }

    return checksum;
}


/* Generate mock NMEA GPGGA sentence */
int gps_generate_mock_sentence(
    char *buffer,
    size_t buffer_size
)
{
    char payload[128];

    int payload_length;
    int message_length;

    unsigned char checksum;

    time_t current_time;
    struct tm utc_time;

    if (buffer == NULL)
    {
        return -1;
    }

    current_time = time(NULL);

    if (current_time == (time_t)-1)
    {
        return -1;
    }

    if (gmtime_r(&current_time, &utc_time) == NULL)
    {
        return -1;
    }

    /*
     * Generate a mock GPGGA payload.
     *
     * Latitude:  19 degrees 04.5000 minutes North
     * Longitude: 072 degrees 52.3000 minutes East
     * Fix:       GPS fix available
     * Satellites: 8
     * HDOP:      0.9
     * Altitude:  15.5 metres
     */

    payload_length = snprintf(
        payload,
        sizeof(payload),
        "GPGGA,%02d%02d%02d.00,1904.5000,N,"
        "07252.3000,E,1,08,0.9,15.5,M,46.9,M,,",
        utc_time.tm_hour,
        utc_time.tm_min,
        utc_time.tm_sec
    );

    if (payload_length < 0 ||
        (size_t)payload_length >= sizeof(payload))
    {
        return -1;
    }

    /* Calculate checksum over payload */
    checksum = gps_checksum(payload);

    /* Construct complete NMEA sentence */
    message_length = snprintf(
        buffer,
        buffer_size,
        "$%s*%02X\r\n",
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