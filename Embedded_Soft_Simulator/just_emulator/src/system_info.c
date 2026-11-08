#include <stdlib.h>
#include <time.h>

#include "system_info.h"


/* Store the device start time */
static struct timespec device_start_time;


/* Initialize system information module */
int initialize_system_info(void)
{
    if (clock_gettime(CLOCK_MONOTONIC, &device_start_time) == -1)
    {
        return -1;
    }

    return 0;
}


/* Collect system information */
int collect_system_info(SystemInfo *info)
{
    struct timespec current_time;

    if (info == NULL)
    {
        return -1;
    }

    /* Generate mock supply voltage: 3.00 V to 3.60 V */
    info->supply_voltage =
        3.00f + (float)(rand() % 61) / 100.0f;

    /* Capture current monotonic time */
    if (clock_gettime(CLOCK_MONOTONIC, &current_time) == -1)
    {
        return -1;
    }

    /* Calculate device uptime */
    info->uptime_seconds =
        (double)(current_time.tv_sec - device_start_time.tv_sec) +
        (double)(current_time.tv_nsec - device_start_time.tv_nsec)
        / 1000000000.0;

    /* Determine device operational status */
    if (info->supply_voltage >= 3.10f &&
        info->supply_voltage <= 3.50f)
    {
        info->status = DEVICE_STATUS_NORMAL;
    }
    else
    {
        info->status = DEVICE_STATUS_WARNING;
    }

    return 0;
}


/* Convert device status into readable text */
const char *get_device_status_string(DeviceStatus status)
{
    switch (status)
    {
        case DEVICE_STATUS_NORMAL:
            return "NORMAL";

        case DEVICE_STATUS_WARNING:
            return "WARNING";

        default:
            return "UNKNOWN";
    }
}
