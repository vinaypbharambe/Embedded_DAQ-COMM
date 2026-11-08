#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include <stddef.h>

/* GPS data extracted from NMEA GPGGA */
typedef struct
{
    char utc_time[16];

    double latitude;
    char latitude_direction;

    double longitude;
    char longitude_direction;

    int fix_quality;
    int satellites;

    float hdop;
    float altitude;

    float geoid_separation;

    bool has_fix;

} GPSData;


/* Generate a mock NMEA GPGGA sentence */
int gps_generate_mock_sentence(
    char *buffer,
    size_t buffer_size
);

#endif