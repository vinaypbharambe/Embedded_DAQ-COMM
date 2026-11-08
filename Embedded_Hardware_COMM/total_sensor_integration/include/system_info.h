#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdbool.h>

/* Device operational status */
typedef enum
{
    DEVICE_STATUS_NORMAL,
    DEVICE_STATUS_WARNING

} DeviceStatus;


/* System information structure */
typedef struct
{
    float supply_voltage;
    double uptime_seconds;
    DeviceStatus status;

} SystemInfo;


/* Initialize system information module */
int initialize_system_info(void);


/* Collect system information */
int collect_system_info(SystemInfo *info);


/* Convert device status into readable text */
const char *get_device_status_string(DeviceStatus status);

#endif
