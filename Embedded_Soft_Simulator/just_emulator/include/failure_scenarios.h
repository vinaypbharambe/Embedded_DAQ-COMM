#ifndef FAILURE_SCENARIOS_H
#define FAILURE_SCENARIOS_H

#include <stdbool.h>

typedef enum
{
    ERROR_NONE,
    ERROR_DHT11,
    ERROR_MPU6050,
    ERROR_GPS,
    ERROR_COMMUNICATION
} ErrorMode;

bool scenario_communication_available(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
);

bool scenario_dht11_failure(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
);

bool scenario_mpu6050_failure(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
);

bool scenario_gps_fix_loss(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
);

#endif
