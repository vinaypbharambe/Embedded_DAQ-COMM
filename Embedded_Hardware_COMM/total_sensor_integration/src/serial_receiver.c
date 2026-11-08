#include <string.h>

#include "serial_receiver.h"


/* Initialize serial receiver */
void serial_receiver_init(SerialReceiver *receiver)
{
    if (receiver == NULL)
    {
        return;
    }

    memset(receiver, 0, sizeof(*receiver));
}


/* Process one received byte */
int serial_receiver_feed(
    SerialReceiver *receiver,
    char byte,
    char *complete_sentence,
    size_t output_size
)
{
    if (receiver == NULL ||
        complete_sentence == NULL ||
        output_size == 0)
    {
        return -1;
    }

    /* Recover if a new sentence begins unexpectedly */
    if (byte == '$')
    {
        receiver->length = 0;
        receiver->discarding = false;
    }

    /* Ignore bytes belonging to an oversized sentence */
    if (receiver->discarding)
    {
        if (byte == '\n')
        {
            receiver->discarding = false;
            receiver->length = 0;
        }

        return 0;
    }

    /* Ignore bytes until sentence start */
    if (receiver->length == 0 && byte != '$')
    {
        return 0;
    }

    /* Detect end of NMEA sentence */
    if (byte == '\n')
    {
        /* Remove trailing carriage return */
        if (receiver->length > 0 &&
            receiver->sentence[receiver->length - 1] == '\r')
        {
            receiver->length--;
        }

        /* Ensure output buffer is large enough */
        if (receiver->length + 1 > output_size)
        {
            receiver->length = 0;
            return -1;
        }

        /* Copy completed sentence */
        memcpy(
            complete_sentence,
            receiver->sentence,
            receiver->length
        );

        complete_sentence[receiver->length] = '\0';

        /* Reset receiver for next sentence */
        receiver->length = 0;

        return 1;
    }

    /* Check for sentence overflow */
    if (receiver->length >= SERIAL_SENTENCE_MAX - 1)
    {
        receiver->length = 0;
        receiver->discarding = true;

        return -1;
    }

    /* Store received byte */
    receiver->sentence[receiver->length++] = byte;

    return 0;
}
