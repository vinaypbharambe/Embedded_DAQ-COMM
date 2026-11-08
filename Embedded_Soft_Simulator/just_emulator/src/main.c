#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "dht11.h"
#include "mpu6050.h"
#include "gps.h"
#include "nmea_parser.h"
#include "system_info.h"
#include "data_record.h"
#include "serial_channel.h"
#include "serial_receiver.h"
#include "circular_buffer.h"
#include "failure_scenarios.h"

#define SAMPLE_INTERVAL_SECONDS 1
#define GPS_BUFFER_SIZE 128
#define SERIAL_READ_TIMEOUT_MS 500
#define DEMO_RECORDS 20


/* ==================================================
   DISPLAY DATA RECORD
   ================================================== */

static void display_data_record(const DataRecord *record)
{
    char timestamp_string[32];

    struct tm timestamp_info;

    if (record == NULL)
    {
        return;
    }

    if (localtime_r(
            &record->timestamp,
            &timestamp_info
        ) != NULL)
    {
        strftime(
            timestamp_string,
            sizeof(timestamp_string),
            "%Y-%m-%d %H:%M:%S",
            &timestamp_info
        );
    }
    else
    {
        snprintf(
            timestamp_string,
            sizeof(timestamp_string),
            "UNKNOWN"
        );
    }

    printf("\n========================================\n");

    printf("MOCK RECEIVER: DATA RECORD DELIVERED\n");

    printf("Record Number: %lu\n",
           record->sequence_number);

    printf("Timestamp: %s\n",
           timestamp_string);

    printf("----------------------------------------\n");

    printf("DHT11 SENSOR\n");

    if (record->dht11_valid)
    {
        printf("Temperature : %.2f C\n",
               record->dht11.temperature);

        printf("Humidity    : %.2f %%\n",
               record->dht11.humidity);

        printf("Sensor Status: VALID\n");
    }
    else
    {
        printf("Sensor Status: INVALID / UNAVAILABLE\n");
    }

    printf("----------------------------------------\n");

    printf("MPU6050 SENSOR\n");

    if (record->mpu6050_valid)
    {
        printf("Pitch : %.2f degrees\n",
               record->mpu6050.pitch);

        printf("Roll  : %.2f degrees\n",
               record->mpu6050.roll);

        printf("Yaw   : %.2f degrees\n",
               record->mpu6050.yaw);

        printf("Sensor Status: VALID\n");
    }
    else
    {
        printf("Sensor Status: INVALID / UNAVAILABLE\n");
    }

    printf("----------------------------------------\n");

    printf("GPS RECEIVER\n");

    if (record->gps_valid)
    {
        printf("UTC Time  : %s\n",
               record->gps.utc_time);

        printf("Latitude  : %.6f %c\n",
               record->gps.latitude,
               record->gps.latitude_direction);

        printf("Longitude : %.6f %c\n",
               record->gps.longitude,
               record->gps.longitude_direction);

        printf("Fix Quality: %d\n",
               record->gps.fix_quality);

        printf("Satellites : %d\n",
               record->gps.satellites);

        printf("HDOP       : %.2f\n",
               record->gps.hdop);

        printf("Altitude   : %.2f metres\n",
               record->gps.altitude);

        printf("GPS Status: VALID FIX\n");
    }
    else
    {
        printf("GPS Status: NO VALID FIX / UNAVAILABLE\n");
    }

    printf("----------------------------------------\n");

    printf("SYSTEM INFORMATION\n");

    printf("Supply Voltage : %.2f V\n",
           record->system_info.supply_voltage);

    printf("Uptime         : %.2f seconds\n",
           record->system_info.uptime_seconds);

    printf("Device Status  : %s\n",
           get_device_status_string(
               record->system_info.status
           ));

    printf("========================================\n");
}


/* ==================================================
   GPS SERIAL RECEPTION AND PARSING
   ================================================== */

static int receive_gps_data(
    SerialChannel *serial,
    SerialReceiver *receiver,
    GPSData *gps
)
{
    char receive_buffer[GPS_BUFFER_SIZE];

    char complete_sentence[SERIAL_SENTENCE_MAX];

    ssize_t received_length;

    int valid_messages = 0;

    int invalid_messages = 0;

    if (serial == NULL ||
        receiver == NULL ||
        gps == NULL)
    {
        return -1;
    }

    gps->has_fix = false;

    while (1)
    {
        received_length = serial_receive_available(
            serial,
            receive_buffer,
            sizeof(receive_buffer),
            SERIAL_READ_TIMEOUT_MS
        );

        if (received_length == 0)
        {
            break;
        }

        if (received_length < 0)
        {
            return -1;
        }

        for (ssize_t i = 0; i < received_length; i++)
        {
            int result = serial_receiver_feed(
                receiver,
                receive_buffer[i],
                complete_sentence,
                sizeof(complete_sentence)
            );

            if (result == 1)
            {
                if (nmea_parse_gpgga(
                        complete_sentence,
                        gps
                    ) == 0)
                {
                    valid_messages++;

                    printf(
                        "\nGPS NMEA sentence received and parsed\n"
                    );
                }
                else
                {
                    invalid_messages++;

                    gps->has_fix = false;

                    fprintf(
                        stderr,
                        "Invalid GPS NMEA sentence rejected\n"
                    );
                }
            }
            else if (result == -1)
            {
                invalid_messages++;

                gps->has_fix = false;

                fprintf(
                    stderr,
                    "Oversized GPS sentence discarded\n"
                );
            }
        }
    }

    printf(
        "GPS reception: %d valid, %d invalid sentence(s)\n",
        valid_messages,
        invalid_messages
    );

    return 0;
}


/* ==================================================
   MOCK RECEIVER
   ================================================== */

static int mock_receiver_send(const DataRecord *record)
{
    if (record == NULL)
    {
        return -1;
    }

    display_data_record(record);

    return 0;
}


/* ==================================================
   TRANSMIT PENDING BUFFERED RECORDS
   ================================================== */

static int transmit_pending_records(CircularBuffer *buffer)
{
    DataRecord record;

    if (buffer == NULL)
    {
        return -1;
    }

    while (!circular_buffer_is_empty(buffer))
    {
        /*
         * Peek at the oldest record by copying it
         * without removing it from the buffer.
         */
        if (buffer->count == 0)
        {
            break;
        }

        record = buffer->records[buffer->head];

        /*
         * Attempt delivery first.
         */
        if (mock_receiver_send(&record) != 0)
        {
            fprintf(
                stderr,
                "Record delivery failed\n"
            );

            /*
             * Keep the record in the buffer.
             */
            return -1;
        }

        /*
         * Remove record only after successful delivery.
         */
        if (circular_buffer_pop(buffer, &record) != 0)
        {
            fprintf(
                stderr,
                "Buffer retrieval failed\n"
            );

            return -1;
        }
    }

    return 0;
}


/* ==================================================
   WAIT FOR NEXT ACQUISITION CYCLE
   ================================================== */

static int wait_for_next_sample(void)
{
    struct timespec duration;

    duration.tv_sec = SAMPLE_INTERVAL_SECONDS;

    duration.tv_nsec = 0;

    while (nanosleep(&duration, &duration) == -1)
    {
        if (errno != EINTR)
        {
            perror("nanosleep");

            return -1;
        }
    }

    return 0;
}


/* ==================================================
   DISPLAY EXECUTION MODE
   ================================================== */

static void display_execution_mode(
    ErrorMode error_mode,
    bool demo_mode
)
{
    if (demo_mode)
    {
        printf("Execution mode: DEMO\n");
    }
    else
    {
        switch (error_mode)
        {
            case ERROR_DHT11:
                printf("Execution mode: DHT11 ERROR\n");
                break;

            case ERROR_MPU6050:
                printf("Execution mode: MPU6050 ERROR\n");
                break;

            case ERROR_GPS:
                printf("Execution mode: GPS ERROR\n");
                break;

            case ERROR_COMMUNICATION:
                printf("Execution mode: COMMUNICATION ERROR\n");
                break;

            case ERROR_NONE:
            default:
                printf("Execution mode: NORMAL\n");
                break;
        }
    }
}


/* ==================================================
   MAIN FUNCTION
   ================================================== */

int main(int argc, char *argv[])
{
    DHT11Data dht11;

    MPU6050Data mpu6050;

    GPSData gps = {0};

    SystemInfo system;

    DataRecord record;

    SerialChannel serial;

    SerialReceiver receiver;

    CircularBuffer buffer;

    char gps_transmit_buffer[GPS_BUFFER_SIZE];

    int message_length;

    int exit_status = EXIT_SUCCESS;

    bool demo_mode = false;

    bool communication_available;

    ErrorMode error_mode = ERROR_NONE;

    unsigned long sequence_number = 1;


    /* ----------------------------------------------
       SELECT EXECUTION MODE
       ---------------------------------------------- */

    if (argc > 1)
    {
        if (strcmp(argv[1], "--demo") == 0)
        {
            demo_mode = true;
        }
        else if (strcmp(argv[1], "--normal") == 0)
        {
            error_mode = ERROR_NONE;
            demo_mode = false;
        }
        else if (strcmp(argv[1], "--error-dht11") == 0)
        {
            error_mode = ERROR_DHT11;
        }
        else if (strcmp(argv[1], "--error-mpu6050") == 0)
        {
            error_mode = ERROR_MPU6050;
        }
        else if (strcmp(argv[1], "--error-gps") == 0)
        {
            error_mode = ERROR_GPS;
        }
        else if (strcmp(argv[1], "--error-communication") == 0)
        {
            error_mode = ERROR_COMMUNICATION;
        }
        else
        {
            printf(
                "Usage: %s [--normal | --demo | "
                "--error-dht11 | --error-mpu6050 | "
                "--error-gps | --error-communication]\n",
                argv[0]
            );

            return EXIT_FAILURE;
        }
    }


    srand((unsigned int)time(NULL));


    /* ----------------------------------------------
       INITIALIZE SYSTEM INFORMATION
       ---------------------------------------------- */

    if (initialize_system_info() != 0)
    {
        fprintf(
            stderr,
            "System information initialization failed\n"
        );

        return EXIT_FAILURE;
    }


    /* ----------------------------------------------
       INITIALIZE SERIAL CHANNEL
       ---------------------------------------------- */

    if (serial_channel_open(&serial) != 0)
    {
        fprintf(
            stderr,
            "Serial channel initialization failed\n"
        );

        return EXIT_FAILURE;
    }

    serial_receiver_init(&receiver);


    /* ----------------------------------------------
       INITIALIZE CIRCULAR BUFFER
       ---------------------------------------------- */

    circular_buffer_init(&buffer);


    /* ----------------------------------------------
       DISPLAY STARTUP INFORMATION
       ---------------------------------------------- */

    printf("\n========================================\n");

    printf("EMBEDDED DATA ACQUISITION SIMULATOR\n");

    printf("========================================\n");

    display_execution_mode(error_mode, demo_mode);

    printf(
        "Circular buffer capacity: %d records\n",
        BUFFER_CAPACITY
    );

    printf("GPS POSIX serial channel initialized\n");

    printf("Data acquisition started\n");

    if (demo_mode)
    {
        printf(
            "Failure simulation enabled for %d cycles\n",
            DEMO_RECORDS
        );
    }

    printf("========================================\n");


    /* ----------------------------------------------
       MAIN ACQUISITION LOOP
       ---------------------------------------------- */

    while (!demo_mode ||
           sequence_number <= DEMO_RECORDS)
    {
        printf(
            "\n\n######## ACQUISITION CYCLE %lu ########\n",
            sequence_number
        );


        /* ==========================================
           STEP 1: DHT11 DATA ACQUISITION
           ========================================== */

        dht11_read(&dht11);

        if (scenario_dht11_failure(
                sequence_number,
                error_mode,
                demo_mode
            ))
        {
            dht11.temperature = NAN;

            printf(
                "[FAILURE] DHT11 sensor unavailable\n"
            );
        }


        /* ==========================================
           STEP 2: MPU6050 DATA ACQUISITION
           ========================================== */

        mpu6050_read(&mpu6050);

        if (scenario_mpu6050_failure(
                sequence_number,
                error_mode,
                demo_mode
            ))
        {
            mpu6050.pitch = NAN;

            printf(
                "[FAILURE] MPU6050 sensor unavailable\n"
            );
        }


        /* ==========================================
           STEP 3: GPS MESSAGE GENERATION
           ========================================== */

        message_length = gps_generate_mock_sentence(
            gps_transmit_buffer,
            sizeof(gps_transmit_buffer)
        );

        if (message_length < 0)
        {
            fprintf(
                stderr,
                "GPS message generation failed\n"
            );

            gps.has_fix = false;
        }
        else
        {
            if (serial_send(
                    &serial,
                    gps_transmit_buffer,
                    (size_t)message_length
                ) != 0)
            {
                fprintf(
                    stderr,
                    "GPS transmission failed\n"
                );

                gps.has_fix = false;
            }
            else
            {
                if (receive_gps_data(
                        &serial,
                        &receiver,
                        &gps
                    ) != 0)
                {
                    fprintf(
                        stderr,
                        "GPS communication error\n"
                    );

                    gps.has_fix = false;
                }
            }
        }


        /* ==========================================
           STEP 4: SIMULATE GPS FIX LOSS
           ========================================== */

        if (scenario_gps_fix_loss(
                sequence_number,
                error_mode,
                demo_mode
            ))
        {
            gps.has_fix = false;

            printf(
                "[FAILURE] GPS fix unavailable\n"
            );
        }


        /* ==========================================
           STEP 5: COLLECT SYSTEM INFORMATION
           ========================================== */

        if (collect_system_info(&system) != 0)
        {
            fprintf(
                stderr,
                "System information collection failed\n"
            );

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }


        /* ==========================================
           STEP 6: CREATE DATA RECORD
           ========================================== */

        if (create_data_record(
                &record,
                &dht11,
                &mpu6050,
                &gps,
                &system,
                sequence_number
            ) != 0)
        {
            fprintf(
                stderr,
                "Data record creation failed\n"
            );

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }


        /* ==========================================
           STEP 7: INSERT RECORD INTO BUFFER
           ========================================== */

        int push_result = circular_buffer_push(
            &buffer,
            &record
        );

        if (push_result == -1)
        {
            fprintf(
                stderr,
                "Data buffer insertion failed\n"
            );

            exit_status = EXIT_FAILURE;

            goto cleanup;
        }

        if (push_result == 1)
        {
            printf(
                "[BUFFER OVERFLOW] Oldest record dropped\n"
            );

            printf(
                "Total dropped records: %lu\n",
                buffer.dropped_records
            );
        }

        printf(
            "Record %lu collected and buffered\n",
            sequence_number
        );

        printf(
            "Current buffer occupancy: %zu/%d\n",
            circular_buffer_count(&buffer),
            BUFFER_CAPACITY
        );


        /* ==========================================
           STEP 8: CHECK COMMUNICATION STATUS
           ========================================== */

        communication_available =
            scenario_communication_available(
                sequence_number,
                error_mode,
                demo_mode
            );

        if (communication_available)
        {
            printf(
                "[COMMUNICATION] Receiver available\n"
            );

            if (transmit_pending_records(&buffer) != 0)
            {
                fprintf(
                    stderr,
                    "Pending record transmission failed\n"
                );
            }
            else
            {
                printf(
                    "[COMMUNICATION] Pending records delivered\n"
                );
            }
        }
        else
        {
            printf(
                "[COMMUNICATION FAILURE] Receiver unavailable\n"
            );

            printf(
                "Records retained in buffer: %zu\n",
                circular_buffer_count(&buffer)
            );
        }

        printf(
            "\nAcquisition cycle %lu completed\n",
            sequence_number
        );

        sequence_number++;


        /* ------------------------------------------
           STOP AFTER FINAL DEMO CYCLE
           ------------------------------------------ */

        if (demo_mode &&
            sequence_number > DEMO_RECORDS)
        {
            break;
        }


        /* ------------------------------------------
           WAIT FOR NEXT SAMPLE
           ------------------------------------------ */

        if (wait_for_next_sample() != 0)
        {
            exit_status = EXIT_FAILURE;

            goto cleanup;
        }
    }


    /* ==============================================
       STEP 9: FINAL BUFFER STATUS
       ============================================== */

    printf("\n========================================\n");

    printf("SIMULATION COMPLETED\n");

    printf("========================================\n");

    printf(
        "Total records collected: %lu\n",
        sequence_number - 1
    );

    printf(
        "Records remaining in buffer: %zu\n",
        circular_buffer_count(&buffer)
    );

    printf(
        "Total records dropped: %lu\n",
        buffer.dropped_records
    );

    printf("========================================\n");


cleanup:

    serial_channel_close(&serial);

    printf("\nSimulator stopped.\n");

    return exit_status;
}
