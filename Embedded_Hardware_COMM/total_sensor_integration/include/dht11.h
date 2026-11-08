#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>

/* DHT11 temperature and humidity data */
typedef struct
{
    float temperature;
    float humidity;

} DHT11Data;


/* Request and receive DHT11 readings */
int dht11_read(
    int serial_fd,
    DHT11Data *data,
    int timeout_ms
);


/* Validate DHT11 readings */
bool dht11_validate(const DHT11Data *data);

#endif