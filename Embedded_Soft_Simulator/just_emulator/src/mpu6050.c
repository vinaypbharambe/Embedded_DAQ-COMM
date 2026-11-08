#include <stdlib.h>
#include <math.h>

#include "mpu6050.h"


/* Generate mock MPU6050 readings */
void mpu6050_read(MPU6050Data *data)
{
    if (data == NULL)
    {
        return;
    }

    /*
     * Generate mock orientation values.
     *
     * Pitch: -90 to +90 degrees
     * Roll:  -90 to +90 degrees
     * Yaw:     0 to 359.99 degrees
     */

    data->pitch =
        (float)(rand() % 18001 - 9000) / 100.0f;

    data->roll =
        (float)(rand() % 18001 - 9000) / 100.0f;

    data->yaw =
        (float)(rand() % 36000) / 100.0f;
}


/* Validate MPU6050 readings */
bool mpu6050_validate(const MPU6050Data *data)
{
    if (data == NULL)
    {
        return false;
    }

    if (!isfinite(data->pitch) ||
        !isfinite(data->roll) ||
        !isfinite(data->yaw))
    {
        return false;
    }

    if (data->pitch < -90.0f ||
        data->pitch > 90.0f)
    {
        return false;
    }

    if (data->roll < -90.0f ||
        data->roll > 90.0f)
    {
        return false;
    }

    if (data->yaw < 0.0f ||
        data->yaw >= 360.0f)
    {
        return false;
    }

    return true;
}
