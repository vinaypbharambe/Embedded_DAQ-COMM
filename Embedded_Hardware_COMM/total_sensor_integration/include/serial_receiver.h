#ifndef SERIAL_RECEIVER_H
#define SERIAL_RECEIVER_H

#include <stddef.h>
#include <stdbool.h>

#define SERIAL_SENTENCE_MAX 128

/* Serial sentence receiver state */
typedef struct
{
    char sentence[SERIAL_SENTENCE_MAX];

    size_t length;

    bool discarding;

} SerialReceiver;


/* Initialize serial receiver */
void serial_receiver_init(SerialReceiver *receiver);


/*
 * Feed received bytes into the sentence receiver.
 *
 * Returns:
 *  1 = complete sentence received
 *  0 = sentence incomplete
 * -1 = oversized sentence discarded
 */
int serial_receiver_feed(
    SerialReceiver *receiver,
    char byte,
    char *complete_sentence,
    size_t output_size
);

#endif
