#include <stdio.h>
#include <string.h>
#include <math.h>

#include "serial_receiver.h"
#include "nmea_parser.h"

#define VALID_NMEA \
    "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"

typedef struct
{
    int completed;
    int accepted;
    int rejected;
    int oversized;

} TestStats;

static int tests_passed = 0;
static int tests_failed = 0;

/* Record the result of an individual check. */
static void check(int condition, const char *description)
{
    if (condition)
    {
        printf("[PASS] %s\n", description);
        tests_passed++;
    }
    else
    {
        printf("[FAIL] %s\n", description);
        tests_failed++;
    }
}

/*
 * Feed a sequence of bytes into the serial receiver.
 * Every completed sentence is passed to the NMEA parser.
 */
static void feed_bytes(
    SerialReceiver *receiver,
    const char *data,
    size_t length,
    GPSData *gps,
    TestStats *stats
)
{
    char complete_sentence[SERIAL_SENTENCE_MAX];

    for (size_t i = 0; i < length; i++)
    {
        int result = serial_receiver_feed(
            receiver,
            data[i],
            complete_sentence,
            sizeof(complete_sentence)
        );

        if (result == 1)
        {
            stats->completed++;

            if (nmea_parse_gpgga(
                    complete_sentence,
                    gps
                ) == 0)
            {
                stats->accepted++;
            }
            else
            {
                stats->rejected++;
            }
        }
        else if (result == -1)
        {
            stats->oversized++;
        }
    }
}

/* Test 1: Receive and parse one complete sentence. */
static void test_complete_sentence(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    printf("\nTEST 1: Complete NMEA sentence\n");

    serial_receiver_init(&receiver);

    feed_bytes(
        &receiver,
        VALID_NMEA,
        strlen(VALID_NMEA),
        &gps,
        &stats
    );

    check(stats.completed == 1,
          "One complete sentence received");

    check(stats.accepted == 1,
          "NMEA sentence successfully parsed");

    check(strcmp(gps.utc_time, "123519") == 0,
          "UTC time extracted correctly");

    check(fabs(gps.latitude - 48.1173) < 0.00001,
          "Latitude converted correctly");

    check(fabs(gps.longitude - 11.5166667) < 0.00001,
          "Longitude converted correctly");

    check(gps.fix_quality == 1,
          "GPS fix quality extracted correctly");

    check(gps.satellites == 8,
          "Satellite count extracted correctly");

    check(gps.has_fix,
          "GPS fix status is valid");
}

/* Test 2: Receive one sentence in three chunks. */
static void test_chunked_sentence(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    const char *sentence = VALID_NMEA;

    size_t length = strlen(sentence);

    size_t first_chunk = length / 3;
    size_t second_chunk = length / 3;
    size_t third_chunk = length -
                         first_chunk -
                         second_chunk;

    printf("\nTEST 2: Chunked NMEA sentence\n");

    serial_receiver_init(&receiver);

    feed_bytes(
        &receiver,
        sentence,
        first_chunk,
        &gps,
        &stats
    );

    check(stats.completed == 0,
          "First chunk does not produce an incomplete sentence");

    feed_bytes(
        &receiver,
        sentence + first_chunk,
        second_chunk,
        &gps,
        &stats
    );

    check(stats.completed == 0,
          "Second chunk does not produce an incomplete sentence");

    feed_bytes(
        &receiver,
        sentence + first_chunk + second_chunk,
        third_chunk,
        &gps,
        &stats
    );

    check(stats.completed == 1,
          "Third chunk completes the sentence");

    check(stats.accepted == 1,
          "Chunked sentence successfully parsed");
}

/* Test 3: Receive two consecutive sentences. */
static void test_consecutive_sentences(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    char combined[sizeof(VALID_NMEA) * 2];

    printf("\nTEST 3: Consecutive NMEA sentences\n");

    serial_receiver_init(&receiver);

    snprintf(
        combined,
        sizeof(combined),
        "%s%s",
        VALID_NMEA,
        VALID_NMEA
    );

    feed_bytes(
        &receiver,
        combined,
        strlen(combined),
        &gps,
        &stats
    );

    check(stats.completed == 2,
          "Both sentences received");

    check(stats.accepted == 2,
          "Both sentences successfully parsed");
}

/* Test 4: Reject a sentence with an incorrect checksum. */
static void test_invalid_checksum(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    char invalid_sentence[sizeof(VALID_NMEA)];

    printf("\nTEST 4: Invalid NMEA checksum\n");

    serial_receiver_init(&receiver);

    strcpy(invalid_sentence, VALID_NMEA);

    /* Change checksum from 47 to 46. */
    char *checksum = strstr(invalid_sentence, "*47");

    if (checksum != NULL)
    {
        checksum[2] = '6';
    }

    feed_bytes(
        &receiver,
        invalid_sentence,
        strlen(invalid_sentence),
        &gps,
        &stats
    );

    check(stats.completed == 1,
          "Sentence framing completed");

    check(stats.rejected == 1,
          "Incorrect checksum rejected by parser");

    check(stats.accepted == 0,
          "Invalid sentence was not accepted");
}

/* Test 5: An incomplete sentence must not be parsed. */
static void test_incomplete_sentence(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    const char *partial =
        "$GPGGA,123519,4807.038,N,01131.000";

    printf("\nTEST 5: Incomplete NMEA sentence\n");

    serial_receiver_init(&receiver);

    feed_bytes(
        &receiver,
        partial,
        strlen(partial),
        &gps,
        &stats
    );

    check(stats.completed == 0,
          "Incomplete sentence is not reported as complete");

    check(stats.accepted == 0,
          "Incomplete sentence is not parsed");

    check(stats.rejected == 0,
          "Incomplete sentence is not prematurely rejected");
}

/* Test 6: Discard an oversized sentence. */
static void test_oversized_sentence(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    char oversized[160];

    printf("\nTEST 6: Oversized NMEA sentence\n");

    serial_receiver_init(&receiver);

    oversized[0] = '$';

    for (size_t i = 1; i < sizeof(oversized) - 2; i++)
    {
        oversized[i] = 'A';
    }

    oversized[sizeof(oversized) - 2] = '\n';
    oversized[sizeof(oversized) - 1] = '\0';

    feed_bytes(
        &receiver,
        oversized,
        strlen(oversized),
        &gps,
        &stats
    );

    check(stats.oversized == 1,
          "Oversized sentence detected and discarded");

    check(stats.completed == 0,
          "Oversized sentence was not parsed");
}

/* Test 7: Recover after corrupted input. */
static void test_recovery_after_corruption(void)
{
    SerialReceiver receiver;
    GPSData gps = {0};
    TestStats stats = {0};

    char corrupted[sizeof(VALID_NMEA)];
    char combined[sizeof(VALID_NMEA) * 2];

    printf("\nTEST 7: Recovery after corrupted input\n");

    serial_receiver_init(&receiver);

    strcpy(corrupted, VALID_NMEA);

    char *checksum = strstr(corrupted, "*47");

    if (checksum != NULL)
    {
        checksum[2] = '6';
    }

    snprintf(
        combined,
        sizeof(combined),
        "%s%s",
        corrupted,
        VALID_NMEA
    );

    feed_bytes(
        &receiver,
        combined,
        strlen(combined),
        &gps,
        &stats
    );

    check(stats.completed == 2,
          "Both corrupted and valid sentences framed");

    check(stats.rejected == 1,
          "Corrupted sentence rejected");

    check(stats.accepted == 1,
          "Valid sentence accepted after corrupted input");

    check(gps.has_fix,
          "GPS receiver recovered and has a valid fix");
}

int main(void)
{
    printf("========================================\n");
    printf(" SERIAL RECEIVER TEST SUITE\n");
    printf("========================================\n");

    test_complete_sentence();

    test_chunked_sentence();

    test_consecutive_sentences();

    test_invalid_checksum();

    test_incomplete_sentence();

    test_oversized_sentence();

    test_recovery_after_corruption();

    printf("\n========================================\n");
    printf(" TEST SUMMARY\n");
    printf("========================================\n");

    printf("Checks passed : %d\n", tests_passed);
    printf("Checks failed : %d\n", tests_failed);

    if (tests_failed == 0)
    {
        printf("\nALL TESTS PASSED\n");
        return 0;
    }

    printf("\nSOME TESTS FAILED\n");

    return 1;
}
