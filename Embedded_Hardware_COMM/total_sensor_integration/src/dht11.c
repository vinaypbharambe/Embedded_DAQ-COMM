#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "dht11.h"
#include "serial_channel.h"

#define DHT11_REQUEST "GET_DHT11\n"
#define DHT11_RESPONSE_SIZE 64


/* ==============================================
   VALIDATE DHT11 READINGS
   ============================================== */

bool dht11_validate(const DHT11Data *data)
{
    if (data == NULL)
    {
        return false;
    }

    if (!isfinite(data->temperature) ||
        !isfinite(data->humidity))
    {
        return false;
    }

    if (data->temperature < 0.0f ||
        data->temperature > 50.0f)
    {
        return false;
    }

    if (data->humidity < 20.0f ||
        data->humidity > 90.0f)
    {
        return false;
    }

    return true;
}


/* ==============================================
   PARSE ARDUINO RESPONSE
   ============================================== */

static int parse_dht11_response(
    const char *response,
    DHT11Data *data
)
{
    char *end_pointer;

    float temperature;
    float humidity;

    if (response == NULL || data == NULL)
    {
        return -1;
    }

    errno = 0;

    temperature = strtof(response, &end_pointer);

    if (end_pointer == response || errno == ERANGE)
    {
        return -1;
    }

    if (*end_pointer != ',')
    {
        return -1;
    }

    const char *humidity_start = end_pointer + 1;

    errno = 0;

    humidity = strtof(humidity_start, &end_pointer);

    if (end_pointer == humidity_start || errno == ERANGE)
    {
        return -1;
    }

    /* Allow whitespace after humidity */
    while (isspace((unsigned char)*end_pointer))
    {
        end_pointer++;
    }

    /* Reject unexpected trailing characters */
    if (*end_pointer != '\0')
    {
        return -1;
    }

    data->temperature = temperature;
    data->humidity = humidity;

    if (!dht11_validate(data))
    {
        return -1;
    }

    return 0;
}


/* ==============================================
   REQUEST AND RECEIVE DHT11 DATA
   ============================================== */

int dht11_read(
    int serial_fd,
    DHT11Data *data,
    int timeout_ms
)
{
    char response[DHT11_RESPONSE_SIZE];

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

    data->temperature = NAN;
    data->humidity = NAN;

    /* Record acquisition start time */
    if (clock_gettime(CLOCK_MONOTONIC, &start_time) != 0)
    {
        return -1;
    }

    /* Send request to Arduino */
    if (serial_send(
            serial_fd,
            DHT11_REQUEST,
            strlen(DHT11_REQUEST)
        ) < 0)
    {
        fprintf(stderr, "DHT11 request transmission failed\n");
        return -1;
    }

    /* Wait for Arduino response */
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
            fprintf(stderr, "DHT11 response timeout\n");
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
            fprintf(stderr, "DHT11 serial reception failed\n");
            return -1;
        }

        if (received == 0)
        {
            fprintf(stderr, "DHT11 response timeout\n");
            return -1;
        }

        /* Process received bytes */
        for (ssize_t i = 0; i < received; i++)
        {
            char byte = receive_buffer[i];

            if (byte == '\n')
            {
                response[response_length] = '\0';

                if (parse_dht11_response(response, data) != 0)
                {
                    fprintf(stderr, "Invalid DHT11 response\n");
                    return -1;
                }

                return 0;
            }

            if (response_length >= sizeof(response) - 1)
            {
                fprintf(stderr, "DHT11 response too long\n");
                return -1;
            }

            response[response_length++] = byte;
        }
    }
}