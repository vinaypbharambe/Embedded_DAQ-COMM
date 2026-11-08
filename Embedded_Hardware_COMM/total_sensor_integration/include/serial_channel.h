#ifndef SERIAL_CHANNEL_H
#define SERIAL_CHANNEL_H

#include <stddef.h>
#include <sys/types.h>

/* Open and configure a physical serial port */
int serial_channel_open(
    const char *device,
    int baud_rate
);

/* Transmit bytes through the serial port */
int serial_send(
    int fd,
    const char *data,
    size_t length
);

/* Receive available bytes with timeout */
ssize_t serial_receive_available(
    int fd,
    char *buffer,
    size_t buffer_size,
    int timeout_ms
);

/* Close the serial port */
void serial_channel_close(int fd);

#endif