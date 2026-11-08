#include <stdio.h>
#include <string.h>

#include "serial_parser.h"

#define TEST_BUFFER_SIZE 160

static int tests_passed = 0;
static int tests_failed = 0;


/* Print test result */
static void report_test(const char *name, int passed)
{
    if (passed)
    {
        printf("[PASS] %s\n", name);
        tests_passed++;
    }
    else
    {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}


/* Feed a complete string into the parser */
static int feed_message(
    SerialParser *parser,
    const char *data,
    ParsedSensorMessage *message
)
{
    int accepted = 0;

    while (*data != '\0')
    {
        int result = serial_parser_feed(
            parser,
            *data,
            message
        );

        if (result == 1)
        {
            accepted++;
        }

        data++;
    }

    return accepted;
}


/* Generate a valid test message */
static int create_test_message(
    char *buffer,
    size_t buffer_size,
    unsigned long sequence_number
)
{
    SensorData sensor = {
        .temperature = 34.25f,
        .humidity = 72.35f,
        .vibration = 0.85f
    };

    return serial_format_message(
        buffer,
        buffer_size,
        sequence_number,
        &sensor
    );
}


/* Test 1: Complete message */
static void test_complete_message(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char buffer[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    int length = create_test_message(
        buffer,
        sizeof(buffer),
        1
    );

    int accepted = feed_message(
        &parser,
        buffer,
        &message
    );

    int passed =
        length > 0 &&
        accepted == 1 &&
        message.sequence_number == 1 &&
        message.sensor_data.temperature == 34.25f;

    report_test("Complete message", passed);
}


/* Test 2: Message arriving in three chunks */
static void test_chunked_message(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char buffer[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    create_test_message(
        buffer,
        sizeof(buffer),
        2
    );

    size_t length = strlen(buffer);

    size_t first_chunk = length / 3;
    size_t second_chunk = length / 3;

    int accepted = 0;

    /* First chunk */
    for (size_t i = 0; i < first_chunk; i++)
    {
        int result = serial_parser_feed(
            &parser,
            buffer[i],
            &message
        );

        if (result == 1)
        {
            accepted++;
        }
    }

    /* Second chunk */
    for (size_t i = first_chunk;
         i < first_chunk + second_chunk;
         i++)
    {
        int result = serial_parser_feed(
            &parser,
            buffer[i],
            &message
        );

        if (result == 1)
        {
            accepted++;
        }
    }

    /* Third chunk */
    for (size_t i = first_chunk + second_chunk;
         i < length;
         i++)
    {
        int result = serial_parser_feed(
            &parser,
            buffer[i],
            &message
        );

        if (result == 1)
        {
            accepted++;
        }
    }

    int passed =
        accepted == 1 &&
        message.sequence_number == 2;

    report_test("Chunked message", passed);
}


/* Test 3: Two consecutive messages */
static void test_consecutive_messages(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char first[TEST_BUFFER_SIZE];
    char second[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    create_test_message(first, sizeof(first), 3);
    create_test_message(second, sizeof(second), 4);

    int accepted = 0;

    accepted += feed_message(&parser, first, &message);
    accepted += feed_message(&parser, second, &message);

    int passed =
        accepted == 2 &&
        message.sequence_number == 4;

    report_test("Consecutive messages", passed);
}


/* Test 4: Corrupted checksum */
static void test_corrupted_checksum(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char buffer[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    create_test_message(buffer, sizeof(buffer), 5);

    char *checksum = strchr(buffer, '*');

    int rejected = 0;

    if (checksum != NULL && checksum[1] != '\0')
    {
        /* Change one checksum digit */
        checksum[1] = (checksum[1] == '0') ? '1' : '0';

        for (size_t i = 0; buffer[i] != '\0'; i++)
        {
            int result = serial_parser_feed(
                &parser,
                buffer[i],
                &message
            );

            if (result == -1)
            {
                rejected++;
            }
        }
    }

    report_test(
        "Corrupted checksum rejected",
        rejected == 1
    );
}


/* Test 5: Incomplete message */
static void test_incomplete_message(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char buffer[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    create_test_message(buffer, sizeof(buffer), 6);

    size_t length = strlen(buffer);

    int accepted = 0;

    /* Deliberately omit the final newline */
    for (size_t i = 0; i < length - 1; i++)
    {
        int result = serial_parser_feed(
            &parser,
            buffer[i],
            &message
        );

        if (result == 1)
        {
            accepted++;
        }
    }

    report_test(
        "Incomplete message not accepted",
        accepted == 0
    );
}


/* Test 6: Invalid sensor values */
static void test_invalid_sensor_data(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char buffer[TEST_BUFFER_SIZE];

    SensorData invalid_sensor = {
        .temperature = 99.0f,
        .humidity = 72.35f,
        .vibration = 0.85f
    };

    serial_parser_init(&parser);

    int length = serial_format_message(
        buffer,
        sizeof(buffer),
        7,
        &invalid_sensor
    );

    int rejected = 0;

    if (length > 0)
    {
        for (int i = 0; i < length; i++)
        {
            int result = serial_parser_feed(
                &parser,
                buffer[i],
                &message
            );

            if (result == -1)
            {
                rejected++;
            }
        }
    }

    report_test(
        "Invalid sensor data rejected",
        rejected == 1
    );
}


/* Test 7: Oversized message */
static void test_oversized_message(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    int rejected = 0;

    serial_parser_init(&parser);

    /* Start a message */
    serial_parser_feed(&parser, '$', &message);

    /* Send more characters than the parser can store */
    for (int i = 0; i < SERIAL_FRAME_MAX; i++)
    {
        int result = serial_parser_feed(
            &parser,
            'A',
            &message
        );

        if (result == -1)
        {
            rejected++;
        }
    }

    report_test(
        "Oversized message rejected safely",
        rejected == 1
    );
}


/* Test 8: Recovery after corrupted input */
static void test_parser_recovery(void)
{
    SerialParser parser;
    ParsedSensorMessage message;

    char invalid[TEST_BUFFER_SIZE];
    char valid[TEST_BUFFER_SIZE];

    serial_parser_init(&parser);

    create_test_message(invalid, sizeof(invalid), 8);
    create_test_message(valid, sizeof(valid), 9);

    /* Corrupt the first message */
    char *checksum = strchr(invalid, '*');

    if (checksum != NULL)
    {
        checksum[1] = (checksum[1] == '0') ? '1' : '0';
    }

    int accepted = feed_message(
        &parser,
        invalid,
        &message
    );

    accepted += feed_message(
        &parser,
        valid,
        &message
    );

    report_test(
        "Parser recovers after corrupted input",
        accepted == 1 &&
        message.sequence_number == 9
    );
}


int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("       SERIAL PARSER TEST SUITE         \n");
    printf("========================================\n\n");

    test_complete_message();
    test_chunked_message();
    test_consecutive_messages();
    test_corrupted_checksum();
    test_incomplete_message();
    test_invalid_sensor_data();
    test_oversized_message();
    test_parser_recovery();

    printf("\n========================================\n");
    printf("              TEST SUMMARY              \n");
    printf("========================================\n");

    printf("Tests Passed : %d\n", tests_passed);
    printf("Tests Failed : %d\n", tests_failed);
    printf("Total Tests  : %d\n", tests_passed + tests_failed);

    printf("========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
