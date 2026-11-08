#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <poll.h>

#include "serial_channel.h"


/* Convert integer baud rate to termios constant */
static speed_t get_baud_rate(int baud_rate)
{
    switch (baud_rate)
    {
        case 9600:
            return B9600;

        case 19200:
            return B19200;

        case 38400:
            return B38400;

        case 57600:
            return B57600;

        case 115200:
            return B115200;

        default:
            return 0;
    }
}


/* Open and configure physical serial port */
int serial_channel_open(
    const char *device,
    int baud_rate
)
{
    int fd;

    struct termios settings;

    speed_t speed = get_baud_rate(baud_rate);

    if (device == NULL || speed == 0)
    {
        fprintf(stderr, "Invalid serial device or baud rate\n");
        return -1;
    }

    /* Open physical serial device */
    fd = open(
        device,
        O_RDWR | O_NOCTTY | O_NONBLOCK
    );

    if (fd == -1)
    {
        perror("Serial port open failed");
        return -1;
    }

    /* Read existing terminal settings */
    if (tcgetattr(fd, &settings) == -1)
    {
        perror("tcgetattr failed");
        close(fd);
        return -1;
    }

    /* Configure raw serial communication */
    cfmakeraw(&settings);

    /* Configure baud rate */
    if (cfsetispeed(&settings, speed) == -1 ||
        cfsetospeed(&settings, speed) == -1)
    {
        perror("Baud rate configuration failed");
        close(fd);
        return -1;
    }

    /* Configure 8N1 communication */
    settings.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    settings.c_cflag |= CS8 | CREAD | CLOCAL;

    /* Disable hardware and software flow control */
    settings.c_cflag &= ~CRTSCTS;
    settings.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* Non-blocking read configuration */
    settings.c_cc[VMIN] = 0;
    settings.c_cc[VTIME] = 0;

    /* Apply configuration */
    if (tcsetattr(fd, TCSANOW, &settings) == -1)
    {
        perror("tcsetattr failed");
        close(fd);
        return -1;
    }

    /* Apply file descriptor flags */
    if (fcntl(fd, F_SETFL, O_RDWR | O_NOCTTY | O_NONBLOCK) == -1)
    {
        perror("fcntl failed");
        close(fd);
        return -1;
    }

    /* Clear old incoming and outgoing data */
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
        perror("tcflush failed");
        close(fd);
        return -1;
    }

    printf("Serial port opened: %s at %d baud\n",
           device,
           baud_rate);

    return fd;
}


/* Transmit all bytes through serial port */
int serial_send(
    int fd,
    const char *data,
    size_t length
)
{
    size_t total_written = 0;

    if (fd < 0 || data == NULL)
    {
        return -1;
    }

    while (total_written < length)
    {
        ssize_t bytes_written = write(
            fd,
            data + total_written,
            length - total_written
        );

        if (bytes_written == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                struct pollfd pfd = {
                    .fd = fd,
                    .events = POLLOUT,
                    .revents = 0
                };

                int result = poll(&pfd, 1, 500);

                if (result > 0 &&
                    (pfd.revents & POLLOUT))
                {
                    continue;
                }
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


/* Receive available serial bytes */
ssize_t serial_receive_available(
    int fd,
    char *buffer,
    size_t buffer_size,
    int timeout_ms
)
{
    struct pollfd pfd;

    int poll_result;

    ssize_t bytes_received;

    if (fd < 0 ||
        buffer == NULL ||
        buffer_size == 0 ||
        timeout_ms < 0)
    {
        return -1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    /* Wait for incoming serial data */
    do
    {
        poll_result = poll(&pfd, 1, timeout_ms);

    } while (poll_result == -1 && errno == EINTR);

    if (poll_result == -1)
    {
        perror("Serial poll failed");
        return -1;
    }

    /* No data received within timeout */
    if (poll_result == 0)
    {
        return 0;
    }

    /* Check communication errors */
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
    {
        errno = EIO;
        perror("Serial communication error");
        return -1;
    }

    if (!(pfd.revents & POLLIN))
    {
        return 0;
    }

    /* Read incoming bytes */
    do
    {
        bytes_received = read(
            fd,
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


/* Close serial port */
void serial_channel_close(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}