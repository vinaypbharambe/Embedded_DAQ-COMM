#include <stdlib.h>
#include <math.h>

#include "dht11.h"


/* Generate mock DHT11 readings */
void dht11_read(DHT11Data *data)
{
    if (data == NULL)
    {
        return;
    }

    /*
     * DHT11 mock characteristics:
     *
     * Temperature: 20 to 45 degrees Celsius
     * Humidity:    30 to 90 percent
     *
     * DHT11 has integer resolution, so mock readings
     * are generated as whole numbers.
     */

    data->temperature = (float)(20 + rand() % 26);

    data->humidity = (float)(30 + rand() % 61);
}


/* Validate DHT11 readings */
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
