# ==============================================
# COMPILER CONFIGURATION
# ==============================================

CC = gcc

CFLAGS = -Wall -Wextra -Werror -std=c11

CPPFLAGS = -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -Iinclude

LDLIBS = -lm


# ==============================================
# EXECUTABLES
# ==============================================

TARGET = embedded_simulator

SERIAL_TEST = test_serial_receiver

BUFFER_TEST = test_circular_buffer


# ==============================================
# MAIN APPLICATION SOURCE FILES
# ==============================================

SRCS = src/main.c \
       src/dht11.c \
       src/mpu6050.c \
       src/gps.c \
       src/nmea_parser.c \
       src/system_info.c \
       src/data_record.c \
       src/serial_channel.c \
       src/serial_receiver.c \
       src/circular_buffer.c

OBJS = $(SRCS:.c=.o)


# ==============================================
# TEST SOURCE FILES
# ==============================================

SERIAL_TEST_SRCS = tests/test_serial_receiver.c \
                   src/serial_receiver.c \
                   src/nmea_parser.c

BUFFER_TEST_SRCS = tests/test_circular_buffer.c \
                   src/circular_buffer.c


# ==============================================
# DEFAULT TARGET
# ==============================================

all: $(TARGET)


# ==============================================
# BUILD APPLICATION
# ==============================================

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDLIBS)


%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


# ==============================================
# HEADER DEPENDENCIES
# ==============================================

src/main.o: include/dht11.h \
            include/mpu6050.h \
            include/gps.h \
            include/nmea_parser.h \
            include/system_info.h \
            include/data_record.h \
            include/serial_channel.h \
            include/serial_receiver.h \
            include/circular_buffer.h

src/dht11.o: include/dht11.h \
             include/serial_channel.h

src/mpu6050.o: include/mpu6050.h \
               include/serial_channel.h

src/gps.o: include/gps.h \
           include/nmea_parser.h \
           include/serial_channel.h \
           include/serial_receiver.h

src/nmea_parser.o: include/nmea_parser.h \
                   include/gps.h

src/system_info.o: include/system_info.h

src/data_record.o: include/data_record.h \
                   include/dht11.h \
                   include/mpu6050.h \
                   include/gps.h \
                   include/system_info.h

src/serial_channel.o: include/serial_channel.h

src/serial_receiver.o: include/serial_receiver.h

src/circular_buffer.o: include/circular_buffer.h \
                       include/data_record.h


# ==============================================
# BUILD TESTS
# ==============================================

$(SERIAL_TEST): $(SERIAL_TEST_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SERIAL_TEST_SRCS) -o $(SERIAL_TEST) $(LDLIBS)


$(BUFFER_TEST): $(BUFFER_TEST_SRCS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BUFFER_TEST_SRCS) -o $(BUFFER_TEST) $(LDLIBS)


test: $(SERIAL_TEST) $(BUFFER_TEST)
	./$(SERIAL_TEST)
	./$(BUFFER_TEST)


# ==============================================
# CLEAN
# ==============================================

clean:
	rm -f $(OBJS) \
	      src/failure_scenarios.o \
	      $(TARGET) \
	      $(SERIAL_TEST) \
	      $(BUFFER_TEST)


.PHONY: all test clean
