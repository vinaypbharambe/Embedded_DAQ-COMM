#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include "gps.h"

/* Maximum NMEA sentence length */
#define NMEA_MAX_LENGTH 128


/* Parse and validate an NMEA GPGGA sentence */
int nmea_parse_gpgga(
    const char *sentence,
    GPSData *gps
);

#endif