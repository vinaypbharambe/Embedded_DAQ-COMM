#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <poll.h>

#include "serial_channel.h"

/* Create and initialize simulated serial channel */
int serial_channel_open(SerialChannel *channel)
{
    char *slave_name;

    struct termios settings;

    int flags;

    if (channel == NULL)
    {
        return -1;
    }

    channel->master_fd = -1;
    channel->slave_fd = -1;

    /* Create pseudo-terminal master */
    channel->master_fd = posix_openpt(O_RDWR | O_NOCTTY);

    if (channel->master_fd == -1)
    {
        perror("posix_openpt");
        return -1;
    }

    /* Grant permission to access slave */
    if (grantpt(channel->master_fd) == -1)
    {
        perror("grantpt");
        serial_channel_close(channel);
        return -1;
    }

    /* Unlock slave device */
    if (unlockpt(channel->master_fd) == -1)
    {
        perror("unlockpt");
        serial_channel_close(channel);
        return -1;
    }

    /* Obtain slave device name */
    slave_name = ptsname(channel->master_fd);

    if (slave_name == NULL)
    {
        perror("ptsname");
        serial_channel_close(channel);
        return -1;
    }

    /* Open pseudo-terminal slave */
    channel->slave_fd = open(
        slave_name,
        O_RDWR | O_NOCTTY
    );

    if (channel->slave_fd == -1)
    {
        perror("open slave");
        serial_channel_close(channel);
        return -1;
    }

    /* Configure slave as raw serial channel */
    if (tcgetattr(channel->slave_fd, &settings) == -1)
    {
        perror("tcgetattr");
        serial_channel_close(channel);
        return -1;
    }

    /* Disable terminal processing */
    settings.c_iflag = 0;
    settings.c_oflag = 0;
    settings.c_lflag = 0;

    /* Configure 8-bit character transmission */
    settings.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    settings.c_cflag |= CS8 | CREAD | CLOCAL;

    /* Configure non-blocking terminal reads */
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;

    if (tcsetattr(
            channel->slave_fd,
            TCSANOW,
            &settings
        ) == -1)
    {
        perror("tcsetattr");
        serial_channel_close(channel);
        return -1;
    }

    /* Enable non-blocking reads */
    flags = fcntl(channel->slave_fd, F_GETFL);

    if (flags == -1)
    {
        perror("fcntl F_GETFL");
        serial_channel_close(channel);
        return -1;
    }

    if (fcntl(
            channel->slave_fd,
            F_SETFL,
            flags | O_NONBLOCK
        ) == -1)
    {
        perror("fcntl F_SETFL");
        serial_channel_close(channel);
        return -1;
    }

    return 0;
}


/* Transmit all bytes through serial channel */
int serial_send(
    SerialChannel *channel,
    const char *data,
    size_t length
)
{
    size_t total_written = 0;

    if (channel == NULL ||
        data == NULL ||
        channel->master_fd == -1)
    {
        return -1;
    }

    while (total_written < length)
    {
        ssize_t bytes_written = write(
            channel->master_fd,
            data + total_written,
            length - total_written
        );

        if (bytes_written == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("Serial transmission failed");
            return -1;
        }

        if (bytes_written == 0)
        {
            return -1;
        }

        total_written += (size_t)bytes_written;
    }

    return 0;
}


/* Wait for and receive currently available serial bytes */
ssize_t serial_receive_available(
    SerialChannel *channel,
    char *buffer,
    size_t buffer_size,
    int timeout_ms
)
{
    struct pollfd poll_descriptor;

    int poll_result;

    ssize_t bytes_received;

    if (channel == NULL ||
        buffer == NULL ||
        buffer_size == 0 ||
        channel->slave_fd == -1 ||
        timeout_ms < 0)
    {
        return -1;
    }

    poll_descriptor.fd = channel->slave_fd;
    poll_descriptor.events = POLLIN;
    poll_descriptor.revents = 0;

    /* Wait for incoming data */
    do
    {
        poll_result = poll(
            &poll_descriptor,
            1,
            timeout_ms
        );

    } while (poll_result == -1 && errno == EINTR);

    if (poll_result == -1)
    {
        perror("poll");
        return -1;
    }

    /* Timeout: no data received */
    if (poll_result == 0)
    {
        return 0;
    }

    /* Detect terminal errors */
    if (poll_descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        errno = EIO;
        return -1;
    }

    if (!(poll_descriptor.revents & POLLIN))
    {
        return 0;
    }

    /* Read currently available bytes */
    do
    {
        bytes_received = read(
            channel->slave_fd,
            buffer,
            buffer_size
        );

    } while (bytes_received == -1 && errno == EINTR);

    if (bytes_received == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }

        perror("Serial reception failed");
        return -1;
    }

    return bytes_received;
}


/* Close serial channel */
void serial_channel_close(SerialChannel *channel)
{
    if (channel == NULL)
    {
        return;
    }

    if (channel->slave_fd != -1)
    {
        close(channel->slave_fd);
        channel->slave_fd = -1;
    }

    if (channel->master_fd != -1)
    {
        close(channel->master_fd);
        channel->master_fd = -1;
    }
}
