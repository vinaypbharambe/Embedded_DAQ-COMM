#ifndef DHT11_H
#define DHT11_H

#include <stdbool.h>

/* DHT11 temperature and humidity data */
typedef struct
{
    float temperature;
    float humidity;

} DHT11Data;


/* Generate mock DHT11 readings */
void dht11_read(DHT11Data *data);


/* Validate DHT11 readings */
bool dht11_validate(const DHT11Data *data);

#endif
