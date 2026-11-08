#ifndef SERIAL_CHANNEL_H
#define SERIAL_CHANNEL_H

#include <stddef.h>
#include <sys/types.h>

/* Simulated serial channel */
typedef struct
{
    int master_fd;
    int slave_fd;

} SerialChannel;


/* Create and initialize simulated serial channel */
int serial_channel_open(SerialChannel *channel);


/* Transmit data through the serial channel */
int serial_send(
    SerialChannel *channel,
    const char *data,
    size_t length
);


/*
 * Wait for serial data and read currently available bytes.
 *
 * Returns:
 *  > 0 : Number of bytes received
 *    0 : Timeout, no data available
 *   -1 : Communication error
 */
ssize_t serial_receive_available(
    SerialChannel *channel,
    char *buffer,
    size_t buffer_size,
    int timeout_ms
);


/* Close the serial channel */
void serial_channel_close(SerialChannel *channel);

#endif
