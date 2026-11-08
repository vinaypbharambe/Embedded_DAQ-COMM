#include "failure_scenarios.h"


/* =========================================
   COMMUNICATION FAILURE
   ========================================= */

bool scenario_communication_available(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
)
{
    /* Individual communication failure */
    if (error_mode == ERROR_COMMUNICATION)
        return false;

    /* Existing demo behaviour */
    if (demo_mode)
    {
        return !(sequence_number >= 5 &&
                 sequence_number <= 11);
    }

    return true;
}


/* =========================================
   DHT11 SENSOR FAILURE
   ========================================= */

bool scenario_dht11_failure(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
)
{
    /* Individual DHT11 failure */
    if (error_mode == ERROR_DHT11)
        return true;

    /* Existing demo behaviour */
    if (demo_mode)
    {
        return sequence_number == 4 ||
               sequence_number == 15;
    }

    return false;
}


/* =========================================
   MPU6050 SENSOR FAILURE
   ========================================= */

bool scenario_mpu6050_failure(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
)
{
    /* Individual MPU6050 failure */
    if (error_mode == ERROR_MPU6050)
        return true;

    /* Existing demo behaviour */
    if (demo_mode)
    {
        return sequence_number == 7 ||
               sequence_number == 16;
    }

    return false;
}


/* =========================================
   GPS FIX LOSS
   ========================================= */

bool scenario_gps_fix_loss(
    unsigned long sequence_number,
    ErrorMode error_mode,
    bool demo_mode
)
{
    /* Individual GPS failure */
    if (error_mode == ERROR_GPS)
        return true;

    /* Existing demo behaviour */
    if (demo_mode)
    {
        return sequence_number == 9 ||
               sequence_number == 17;
    }

    return false;
}
