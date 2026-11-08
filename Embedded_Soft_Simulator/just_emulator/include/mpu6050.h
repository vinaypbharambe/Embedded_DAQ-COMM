#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>

/* MPU6050 orientation data */
typedef struct
{
    float pitch;
    float roll;
    float yaw;

} MPU6050Data;


/* Generate mock MPU6050 readings */
void mpu6050_read(MPU6050Data *data);


/* Validate MPU6050 readings */
bool mpu6050_validate(const MPU6050Data *data);

#endif
