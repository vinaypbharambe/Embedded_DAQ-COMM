#include <stdio.h>
#include <string.h>

#include "gps.h"
#include "nmea_parser.h"
#include "serial_channel.h"

#define GPS_BUFFER_SIZE 128


/* ==============================================
   RECEIVE AND PARSE GPS DATA
   ============================================== */

int gps_receive(
    int serial_fd,
    SerialReceiver *receiver,
    GPSData *gps,
    int timeout_ms
)
{
    char receive_buffer[GPS_BUFFER_SIZE];

    char complete_sentence[SERIAL_SENTENCE_MAX];

    int valid_messages = 0;
    int invalid_messages = 0;

    if (serial_fd < 0 ||
        receiver == NULL ||
        gps == NULL ||
        timeout_ms < 0)
    {
        return -1;
    }

    /*
     * Read available GPS bytes.
     *
     * The timeout applies to waiting for the first
     * available bytes. Once data arrives, continue
     * collecting immediately available bytes.
     */

    ssize_t received_length = serial_receive_available(
        serial_fd,
        receive_buffer,
        sizeof(receive_buffer),
        timeout_ms
    );

    if (received_length < 0)
    {
        fprintf(stderr, "GPS serial reception error\n");
        return -1;
    }

    if (received_length == 0)
    {
        return 0;
    }

    /* Process every received byte */

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
            GPSData parsed_gps = {0};

            if (nmea_parse_gpgga(
                    complete_sentence,
                    &parsed_gps
                ) == 0)
            {
                /*
                 * Update GPS data only after
                 * successful parsing.
                 */

                *gps = parsed_gps;

                valid_messages++;

                printf("GPS GPGGA sentence received successfully\n");
            }
            else
            {
                invalid_messages++;

                fprintf(stderr, "Invalid GPS GPGGA sentence\n");
            }
        }
        else if (result == -1)
        {
            invalid_messages++;

            fprintf(stderr, "Oversized GPS sentence discarded\n");
        }
    }

    if (invalid_messages > 0)
    {
        fprintf(
            stderr,
            "GPS invalid sentences: %d\n",
            invalid_messages
        );
    }

    return valid_messages;
}