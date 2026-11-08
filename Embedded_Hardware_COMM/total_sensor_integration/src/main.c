#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

#include "dht11.h"
#include "mpu6050.h"
#include "gps.h"
#include "system_info.h"
#include "data_record.h"
#include "serial_channel.h"
#include "serial_receiver.h"
#include "circular_buffer.h"


/* ==============================================
   CONFIGURATION
   ============================================== */

#define SAMPLE_INTERVAL_SECONDS 1

#define ARDUINO_BAUD 9600
#define STM32_BAUD 115200
#define GPS_BAUD 9600

#define SENSOR_TIMEOUT_MS 1000
#define GPS_READ_TIMEOUT_MS 0

#define RECORD_MESSAGE_SIZE 512


/* ==============================================
   GLOBAL VARIABLES
   ============================================== */

static volatile sig_atomic_t running = 1;


/* ==============================================
   HANDLE CTRL+C
   ============================================== */

static void handle_signal(int signal_number)
{
    (void)signal_number;

    running = 0;
}


/* ==============================================
   WAIT FOR NEXT ACQUISITION CYCLE
   ============================================== */

static int wait_for_next_sample(void)
{
    struct timespec duration = {
        .tv_sec = SAMPLE_INTERVAL_SECONDS,
        .tv_nsec = 0
    };

    while (nanosleep(&duration, &duration) == -1)
    {
        if (errno != EINTR)
        {
            perror("nanosleep");
            return -1;
        }

        if (!running)
        {
            break;
        }
    }

    return 0;
}


/* ==============================================
   WAIT FOR ARDUINO STARTUP
   ============================================== */

static void wait_for_arduino_startup(void)
{
    struct timespec delay = {
        .tv_sec = 2,
        .tv_nsec = 0
    };

    while (nanosleep(&delay, &delay) == -1)
    {
        if (errno != EINTR)
        {
            perror("Arduino startup delay");
            break;
        }
    }
}


/* ==============================================
   DISPLAY DATA RECORD
   ============================================== */

static void display_data_record(const DataRecord *record)
{
    if (record == NULL)
    {
        return;
    }

    printf("\n========================================\n");
    printf("DATA ACQUISITION RECORD\n");
    printf("========================================\n");

    printf("Sequence Number: %lu\n",
           record->sequence_number);

    printf("Timestamp: %lld\n",
           (long long)record->timestamp);

    printf("----------------------------------------\n");

    printf("DHT11\n");

    if (record->dht11_valid)
    {
        printf("Temperature: %.2f C\n",
               record->dht11.temperature);

        printf("Humidity: %.2f %%\n",
               record->dht11.humidity);
    }
    else
    {
        printf("DHT11 unavailable or invalid\n");
    }

    printf("----------------------------------------\n");

    printf("MPU6050\n");

    if (record->mpu6050_valid)
    {
        printf("Pitch: %.2f degrees\n",
               record->mpu6050.pitch);

        printf("Roll: %.2f degrees\n",
               record->mpu6050.roll);

        printf("Yaw: %.2f degrees\n",
               record->mpu6050.yaw);
    }
    else
    {
        printf("MPU6050 unavailable or invalid\n");
    }

    printf("----------------------------------------\n");

    printf("GPS\n");

    if (record->gps_valid)
    {
        printf("UTC: %s\n",
               record->gps.utc_time);

        printf("Latitude: %.6f %c\n",
               record->gps.latitude,
               record->gps.latitude_direction);

        printf("Longitude: %.6f %c\n",
               record->gps.longitude,
               record->gps.longitude_direction);

        printf("Fix quality: %d\n",
               record->gps.fix_quality);

        printf("Satellites: %d\n",
               record->gps.satellites);

        printf("HDOP: %.2f\n",
               record->gps.hdop);

        printf("Altitude: %.2f metres\n",
               record->gps.altitude);
    }
    else
    {
        printf("GPS fix unavailable\n");
    }

    printf("----------------------------------------\n");

    printf("SYSTEM INFORMATION\n");

    printf("Supply Voltage: %.2f V\n",
           record->system_info.supply_voltage);

    printf("Uptime: %.2f seconds\n",
           record->system_info.uptime_seconds);

    printf("Device Status: %s\n",
           get_device_status_string(
               record->system_info.status
           ));

    printf("========================================\n");
}


/* ==============================================
   FORMAT DATA RECORD FOR ARDUINO
   ============================================== */

static int format_data_record(
    const DataRecord *record,
    char *message,
    size_t message_size
)
{
    if (record == NULL ||
        message == NULL ||
        message_size == 0)
    {
        return -1;
    }

    int length = snprintf(
        message,
        message_size,

        "DATA,%lu,%lld,"
        "%d,%.2f,%.2f,"
        "%d,%.2f,%.2f,%.2f,"
        "%d,%s,%.6f,%c,%.6f,%c,"
        "%d,%d,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%d\n",

        record->sequence_number,
        (long long)record->timestamp,

        record->dht11_valid,
        record->dht11.temperature,
        record->dht11.humidity,

        record->mpu6050_valid,
        record->mpu6050.pitch,
        record->mpu6050.roll,
        record->mpu6050.yaw,

        record->gps_valid,
        record->gps.utc_time,
        record->gps.latitude,
        record->gps.latitude_direction,
        record->gps.longitude,
        record->gps.longitude_direction,

        record->gps.fix_quality,
        record->gps.satellites,
        record->gps.hdop,
        record->gps.altitude,
        record->gps.geoid_separation,

        record->system_info.supply_voltage,
        record->system_info.uptime_seconds,
        (int)record->system_info.status
    );

    if (length < 0 || (size_t)length >= message_size)
    {
        return -1;
    }

    return length;
}


/* ==============================================
   TRANSMIT PENDING RECORDS TO ARDUINO
   ============================================== */

static int transmit_pending_records(
    CircularBuffer *buffer,
    int arduino_fd
)
{
    DataRecord record;

    char message[RECORD_MESSAGE_SIZE];

    if (buffer == NULL || arduino_fd < 0)
    {
        return -1;
    }

    while (!circular_buffer_is_empty(buffer))
    {
        /*
         * Read oldest record without removing it.
         */

        record = buffer->records[buffer->head];

        /*
         * Convert record into a serial message.
         */

        int message_length = format_data_record(
            &record,
            message,
            sizeof(message)
        );

        if (message_length < 0)
        {
            fprintf(stderr, "Record formatting failed\n");
            return -1;
        }

        /*
         * Transmit to Arduino.
         */

        if (serial_send(
                arduino_fd,
                message,
                (size_t)message_length
            ) < 0)
        {
            fprintf(stderr, "Arduino record transmission failed\n");
            return -1;
        }

        printf(
            "Record %lu transmitted to Arduino\n",
            record.sequence_number
        );

        /*
         * Remove record after successful serial write.
         */

        if (circular_buffer_pop(buffer, &record) != 0)
        {
            fprintf(stderr, "Circular buffer retrieval failed\n");
            return -1;
        }
    }

    return 0;
}


/* ==============================================
   MAIN FUNCTION
   ============================================== */

int main(int argc, char *argv[])
{
    int arduino_fd = -1;
    int stm32_fd = -1;
    int gps_fd = -1;

    DHT11Data dht11 = {
        .temperature = NAN,
        .humidity = NAN
    };

    MPU6050Data mpu6050 = {
        .pitch = NAN,
        .roll = NAN,
        .yaw = NAN
    };

    GPSData gps = {0};

    SystemInfo system;

    DataRecord record;

    SerialReceiver gps_receiver;

    CircularBuffer buffer;

    unsigned long sequence_number = 1;

    int exit_status = EXIT_SUCCESS;


    /* ==========================================
       CHECK COMMAND-LINE ARGUMENTS
       ========================================== */

    if (argc != 4)
    {
        printf(
            "Usage: %s <arduino_port> <stm32_port> <gps_port>\n",
            argv[0]
        );

        printf("Use '-' for an unavailable device.\n");

        printf(
            "Example: %s /dev/ttyUSB0 - -\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }


    /* ==========================================
       REGISTER SIGNAL HANDLER
       ========================================== */

    signal(SIGINT, handle_signal);


    /* ==========================================
       INITIALIZE SYSTEM INFORMATION
       ========================================== */

    if (initialize_system_info() != 0)
    {
        fprintf(stderr, "System initialization failed\n");

        return EXIT_FAILURE;
    }


    /* ==========================================
       OPEN ARDUINO SERIAL CONNECTION
       ========================================== */

    if (strcmp(argv[1], "-") != 0)
    {
        printf("Opening Arduino: %s\n", argv[1]);

        arduino_fd = serial_channel_open(
            argv[1],
            ARDUINO_BAUD
        );

        if (arduino_fd < 0)
        {
            fprintf(stderr, "Arduino connection failed\n");

            goto cleanup;
        }

        /*
         * Arduino Uno may reset when USB serial opens.
         */

        printf("Waiting for Arduino startup...\n");

        wait_for_arduino_startup();
    }
    else
    {
        printf("Arduino disabled\n");
    }


    /* ==========================================
       OPEN STM32 SERIAL CONNECTION
       ========================================== */

    if (strcmp(argv[2], "-") != 0)
    {
        printf("Opening STM32: %s\n", argv[2]);

        stm32_fd = serial_channel_open(
            argv[2],
            STM32_BAUD
        );

        if (stm32_fd < 0)
        {
            fprintf(stderr, "STM32 connection failed\n");

            goto cleanup;
        }
    }
    else
    {
        printf("STM32 disabled\n");
    }


    /* ==========================================
       OPEN GPS SERIAL CONNECTION
       ========================================== */

    if (strcmp(argv[3], "-") != 0)
    {
        printf("Opening GPS: %s\n", argv[3]);

        gps_fd = serial_channel_open(
            argv[3],
            GPS_BAUD
        );

        if (gps_fd < 0)
        {
            fprintf(stderr, "GPS connection failed\n");

            goto cleanup;
        }
    }
    else
    {
        printf("GPS disabled\n");
    }


    /* ==========================================
       INITIALIZE GPS RECEIVER
       ========================================== */

    serial_receiver_init(&gps_receiver);


    /* ==========================================
       INITIALIZE CIRCULAR BUFFER
       ========================================== */

    circular_buffer_init(&buffer);


    /* ==========================================
       DISPLAY STARTUP INFORMATION
       ========================================== */

    printf("\n========================================\n");
    printf("EMBEDDED DATA ACQUISITION SYSTEM\n");
    printf("========================================\n");

    printf(
        "Arduino: %s\n",
        arduino_fd >= 0 ? argv[1] : "Disabled"
    );

    printf(
        "STM32: %s\n",
        stm32_fd >= 0 ? argv[2] : "Disabled"
    );

    printf(
        "GPS: %s\n",
        gps_fd >= 0 ? argv[3] : "Disabled"
    );

    printf("Buffer capacity: %d records\n", BUFFER_CAPACITY);

    printf("Acquisition started\n");
    printf("Press CTRL+C to stop\n");

    printf("========================================\n");


    /* ==========================================
       MAIN ACQUISITION LOOP
       ========================================== */

    while (running)
    {
        printf(
            "\n\n######## ACQUISITION CYCLE %lu ########\n",
            sequence_number
        );


        /* ======================================
           STEP 1: DHT11 ACQUISITION
           ====================================== */

        if (arduino_fd >= 0)
        {
            printf("\nRequesting DHT11 data...\n");

            if (dht11_read(
                    arduino_fd,
                    &dht11,
                    SENSOR_TIMEOUT_MS
                ) == 0)
            {
                printf(
                    "DHT11: %.2f C, %.2f %%\n",
                    dht11.temperature,
                    dht11.humidity
                );
            }
            else
            {
                printf("DHT11 acquisition failed\n");
            }
        }
        else
        {
            printf("DHT11 skipped: Arduino unavailable\n");
        }


        /* ======================================
           STEP 2: MPU6050 ACQUISITION
           ====================================== */

        if (stm32_fd >= 0)
        {
            printf("\nRequesting MPU6050 data...\n");

            if (mpu6050_read(
                    stm32_fd,
                    &mpu6050,
                    SENSOR_TIMEOUT_MS
                ) == 0)
            {
                printf(
                    "MPU6050: Pitch %.2f, Roll %.2f, Yaw %.2f\n",
                    mpu6050.pitch,
                    mpu6050.roll,
                    mpu6050.yaw
                );
            }
            else
            {
                printf("MPU6050 acquisition failed\n");
            }
        }
        else
        {
            printf("MPU6050 skipped: STM32 unavailable\n");
        }


        /* ======================================
           STEP 3: GPS ACQUISITION
           ====================================== */

        if (gps_fd >= 0)
        {
            printf("\nChecking GPS serial data...\n");

            int gps_result = gps_receive(
                gps_fd,
                &gps_receiver,
                &gps,
                GPS_READ_TIMEOUT_MS
            );

            if (gps_result < 0)
            {
                fprintf(stderr, "GPS reception error\n");
            }
            else if (gps_result > 0)
            {
                printf(
                    "GPS: %d valid GPGGA sentences received\n",
                    gps_result
                );
            }
            else
            {
                printf("No new GPS data available\n");
            }
        }
        else
        {
            printf("GPS skipped: Vicharak unavailable\n");
        }


        /* ======================================
           STEP 4: SYSTEM INFORMATION
           ====================================== */

        printf("\nCollecting system information...\n");

        if (collect_system_info(&system) != 0)
        {
            fprintf(stderr, "System information collection failed\n");

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }


        /* ======================================
           STEP 5: CREATE DATA RECORD
           ====================================== */

        if (create_data_record(
                &record,
                &dht11,
                &mpu6050,
                &gps,
                &system,
                sequence_number
            ) != 0)
        {
            fprintf(stderr, "Data record creation failed\n");

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }


        display_data_record(&record);


        /* ======================================
           STEP 6: INSERT INTO CIRCULAR BUFFER
           ====================================== */

        int push_result = circular_buffer_push(
            &buffer,
            &record
        );

        if (push_result < 0)
        {
            fprintf(stderr, "Buffer insertion failed\n");

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }

        if (push_result == 1)
        {
            printf("[BUFFER WARNING] Oldest record dropped\n");

            printf(
                "Dropped records: %lu\n",
                buffer.dropped_records
            );
        }

        printf(
            "Buffer occupancy: %zu/%d\n",
            circular_buffer_count(&buffer),
            BUFFER_CAPACITY
        );


        /* ======================================
           STEP 7: TRANSMIT RECORD TO ARDUINO
           ====================================== */

        if (arduino_fd >= 0)
        {
            printf("\nTransmitting records to Arduino...\n");

            if (transmit_pending_records(
                    &buffer,
                    arduino_fd
                ) != 0)
            {
                fprintf(
                    stderr,
                    "Record transmission failed; pending records retained\n"
                );
            }
        }
        else
        {
            printf("Record transmission skipped: Arduino unavailable\n");
        }


        printf(
            "\nAcquisition cycle %lu completed\n",
            sequence_number
        );

        sequence_number++;


        /* ======================================
           STEP 8: WAIT FOR NEXT CYCLE
           ====================================== */

        if (running && wait_for_next_sample() != 0)
        {
            exit_status = EXIT_FAILURE;

            goto cleanup;
        }
    }


    printf("\nAcquisition stopped by user\n");


/* ==============================================
   CLEANUP
   ============================================== */

cleanup:

    printf("\nClosing serial connections...\n");

    if (arduino_fd >= 0)
    {
        serial_channel_close(arduino_fd);
    }

    if (stm32_fd >= 0)
    {
        serial_channel_close(stm32_fd);
    }

    if (gps_fd >= 0)
    {
        serial_channel_close(gps_fd);
    }

    printf("Embedded data acquisition system stopped\n");

    return exit_status;
}
