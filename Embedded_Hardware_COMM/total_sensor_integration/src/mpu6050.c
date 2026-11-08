#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "mpu6050.h"
#include "serial_channel.h"

#define MPU6050_REQUEST "GET_MPU6050\n"
#define MPU6050_RESPONSE_SIZE 96


/* ==============================================
   VALIDATE MPU6050 READINGS
   ============================================== */

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


/* ==============================================
   PARSE STM32 RESPONSE
   ============================================== */

static int parse_mpu6050_response(
    const char *response,
    MPU6050Data *data
)
{
    char *end_pointer;

    float pitch;
    float roll;
    float yaw;

    if (response == NULL || data == NULL)
    {
        return -1;
    }

    /* Extract pitch */

    errno = 0;

    pitch = strtof(response, &end_pointer);

    if (end_pointer == response || errno == ERANGE)
    {
        return -1;
    }

    if (*end_pointer != ',')
    {
        return -1;
    }

    /* Extract roll */

    const char *roll_start = end_pointer + 1;

    errno = 0;

    roll = strtof(roll_start, &end_pointer);

    if (end_pointer == roll_start || errno == ERANGE)
    {
        return -1;
    }

    if (*end_pointer != ',')
    {
        return -1;
    }

    /* Extract yaw */

    const char *yaw_start = end_pointer + 1;

    errno = 0;

    yaw = strtof(yaw_start, &end_pointer);

    if (end_pointer == yaw_start || errno == ERANGE)
    {
        return -1;
    }

    /* Allow trailing whitespace */

    while (isspace((unsigned char)*end_pointer))
    {
        end_pointer++;
    }

    if (*end_pointer != '\0')
    {
        return -1;
    }

    data->pitch = pitch;
    data->roll = roll;
    data->yaw = yaw;

    if (!mpu6050_validate(data))
    {
        return -1;
    }

    return 0;
}


/* ==============================================
   REQUEST AND RECEIVE MPU6050 DATA
   ============================================== */

int mpu6050_read(
    int serial_fd,
    MPU6050Data *data,
    int timeout_ms
)
{
    char response[MPU6050_RESPONSE_SIZE];

    char receive_buffer[32];

    size_t response_length = 0;

    struct timespec start_time;
    struct timespec current_time;

    int elapsed_ms;
    int remaining_ms;

    if (serial_fd < 0 || data == NULL || timeout_ms < 0)
    {
        return -1;
    }

    data->pitch = NAN;
    data->roll = NAN;
    data->yaw = NAN;

    /* Record acquisition start time */

    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0)
    {
        return -1;
    }

    /* Send request to STM32 */

    if (serial_send(
            serial_fd,
            MPU6050_REQUEST,
            strlen(MPU6050_REQUEST)
        ) < 0)
    {
        fprintf(stderr, "MPU6050 request transmission failed\n");
        return -1;
    }

    /* Wait for STM32 response */

    while (1)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0)
        {
            return -1;
        }

        elapsed_ms =
            (int)((current_time.tv_sec - start_time.tv_sec) * 1000) +
            (int)((current_time.tv_nsec - start_time.tv_nsec) / 1000000);

        remaining_ms = timeout_ms - elapsed_ms;

        if (remaining_ms <= 0)
        {
            fprintf(stderr, "MPU6050 response timeout\n");
            return -1;
        }

        ssize_t received = serial_receive_available(
            serial_fd,
            receive_buffer,
            sizeof(receive_buffer),
            remaining_ms
        );

        if (received < 0)
        {
            fprintf(stderr, "MPU6050 serial reception failed\n");
            return -1;
        }

        if (received == 0)
        {
            fprintf(stderr, "MPU6050 response timeout\n");
            return -1;
        }

        /* Process received bytes */

        for (ssize_t i = 0; i < received; i++)
        {
            char byte = receive_buffer[i];

            if (byte == '\n')
            {
                response[response_length] = '\0';

                if (parse_mpu6050_response(response, data) != 0)
                {
                    fprintf(stderr, "Invalid MPU6050 response\n");
                    return -1;
                }

                return 0;
            }

            if (response_length >= sizeof(response) - 1)
            {
                fprintf(stderr, "MPU6050 response too long\n");
                return -1;
            }

            response[response_length++] = byte;
        }
    }
}