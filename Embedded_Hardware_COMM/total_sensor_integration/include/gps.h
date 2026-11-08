#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include <stddef.h>

#include "serial_receiver.h"

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


/* Process available GPS serial data */
int gps_receive(
    int serial_fd,
    SerialReceiver *receiver,
    GPSData *gps,
    int timeout_ms
);

#endif