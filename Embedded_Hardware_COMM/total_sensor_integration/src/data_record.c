#include <time.h>

#include "data_record.h"


/* Create a unified data record */
int create_data_record(
    DataRecord *record,
    const DHT11Data *dht11,
    const MPU6050Data *mpu6050,
    const GPSData *gps,
    const SystemInfo *system,
    unsigned long sequence_number
)
{
    if (record == NULL ||
        dht11 == NULL ||
        mpu6050 == NULL ||
        gps == NULL ||
        system == NULL)
    {
        return -1;
    }

    record->sequence_number = sequence_number;

    record->timestamp = time(NULL);

    if (record->timestamp == (time_t)-1)
    {
        return -1;
    }

    record->dht11 = *dht11;

    record->mpu6050 = *mpu6050;

    record->gps = *gps;

    record->system_info = *system;

    record->dht11_valid = dht11_validate(dht11);

    record->mpu6050_valid = mpu6050_validate(mpu6050);

    /*
     * A GPS record is valid only if it was parsed
     * successfully and a position fix is available.
     */
    record->gps_valid = gps->has_fix;

    return 0;
}