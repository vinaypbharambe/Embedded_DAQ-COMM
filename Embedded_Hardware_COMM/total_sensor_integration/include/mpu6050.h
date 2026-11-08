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


/* Request and receive MPU6050 readings */
int mpu6050_read(
    int serial_fd,
    MPU6050Data *data,
    int timeout_ms
);


/* Validate MPU6050 readings */
bool mpu6050_validate(const MPU6050Data *data);

#endif