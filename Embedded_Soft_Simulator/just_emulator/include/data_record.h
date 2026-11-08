#ifndef DATA_RECORD_H
#define DATA_RECORD_H

#include <stdbool.h>
#include <time.h>

#include "dht11.h"
#include "mpu6050.h"
#include "gps.h"
#include "system_info.h"


/* Unified data record */
typedef struct
{
    unsigned long sequence_number;

    time_t timestamp;

    DHT11Data dht11;

    MPU6050Data mpu6050;

    GPSData gps;

    SystemInfo system_info;

    bool dht11_valid;

    bool mpu6050_valid;

    bool gps_valid;

} DataRecord;


/* Create a unified data record */
int create_data_record(
    DataRecord *record,
    const DHT11Data *dht11,
    const MPU6050Data *mpu6050,
    const GPSData *gps,
    const SystemInfo *system,
    unsigned long sequence_number
);

#endif