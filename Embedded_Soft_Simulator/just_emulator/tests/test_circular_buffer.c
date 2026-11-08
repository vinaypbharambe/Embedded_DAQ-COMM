#include <stdio.h>
#include <string.h>

#include "circular_buffer.h"

static int tests_passed = 0;
static int tests_failed = 0;

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

static DataRecord create_test_record(unsigned long sequence)
{
    DataRecord record;

    memset(&record, 0, sizeof(record));

    record.sequence_number = sequence;

    return record;
}

static void test_initialization(void)
{
    CircularBuffer buffer;

    printf("\nTEST 1: Buffer initialization\n");

    circular_buffer_init(&buffer);

    check(circular_buffer_is_empty(&buffer),
          "Buffer initialized as empty");

    check(!circular_buffer_is_full(&buffer),
          "Buffer is not initially full");

    check(circular_buffer_count(&buffer) == 0,
          "Initial record count is zero");
}

static void test_normal_push_pop(void)
{
    CircularBuffer buffer;

    DataRecord input;
    DataRecord output;

    printf("\nTEST 2: Normal insertion and retrieval\n");

    circular_buffer_init(&buffer);

    for (unsigned long i = 1; i <= 3; i++)
    {
        input = create_test_record(i);

        check(circular_buffer_push(&buffer, &input) == 0,
              "Record inserted successfully");
    }

    check(circular_buffer_count(&buffer) == 3,
          "Buffer contains three records");

    for (unsigned long i = 1; i <= 3; i++)
    {
        check(circular_buffer_pop(&buffer, &output) == 0,
              "Record retrieved successfully");

        check(output.sequence_number == i,
              "FIFO order maintained");
    }

    check(circular_buffer_is_empty(&buffer),
          "Buffer empty after retrieval");
}

static void test_empty_buffer(void)
{
    CircularBuffer buffer;

    DataRecord output;

    printf("\nTEST 3: Empty buffer handling\n");

    circular_buffer_init(&buffer);

    check(circular_buffer_pop(&buffer, &output) == -1,
          "Cannot retrieve from empty buffer");
}

static void test_full_buffer(void)
{
    CircularBuffer buffer;

    DataRecord input;

    printf("\nTEST 4: Full buffer handling\n");

    circular_buffer_init(&buffer);

    for (unsigned long i = 1; i <= BUFFER_CAPACITY; i++)
    {
        input = create_test_record(i);

        circular_buffer_push(&buffer, &input);
    }

    check(circular_buffer_is_full(&buffer),
          "Buffer correctly identifies full condition");

    check(circular_buffer_count(&buffer) == BUFFER_CAPACITY,
          "Buffer contains maximum records");
}

static void test_overflow_policy(void)
{
    CircularBuffer buffer;

    DataRecord input;
    DataRecord output;

    int push_result;

    printf("\nTEST 5: Buffer overflow policy\n");

    circular_buffer_init(&buffer);

    for (unsigned long i = 1; i <= BUFFER_CAPACITY; i++)
    {
        input = create_test_record(i);

        circular_buffer_push(&buffer, &input);
    }

    input = create_test_record(6);

    push_result = circular_buffer_push(&buffer, &input);

    check(push_result == 1,
          "Overflow detected");

    check(buffer.dropped_records == 1,
          "Oldest record drop counter incremented");

    check(circular_buffer_count(&buffer) == BUFFER_CAPACITY,
          "Buffer capacity maintained");

    check(circular_buffer_pop(&buffer, &output) == 0,
          "Record retrieved after overflow");

    check(output.sequence_number == 2,
          "Oldest record discarded; next record is 2");

    check(circular_buffer_pop(&buffer, &output) == 0,
          "Next record retrieved");

    check(output.sequence_number == 3,
          "FIFO order maintained after overflow");
}

static void test_multiple_wraparounds(void)
{
    CircularBuffer buffer;

    DataRecord input;
    DataRecord output;

    printf("\nTEST 6: Circular index wraparound\n");

    circular_buffer_init(&buffer);

    /*
     * Repeatedly insert and retrieve records
     * to force head and tail index wraparound.
     */
    for (unsigned long i = 1; i <= 20; i++)
    {
        input = create_test_record(i);

        check(circular_buffer_push(&buffer, &input) == 0,
              "Record inserted");

        check(circular_buffer_pop(&buffer, &output) == 0,
              "Record retrieved");

        check(output.sequence_number == i,
              "Correct record retrieved after wraparound");
    }

    check(circular_buffer_is_empty(&buffer),
          "Buffer empty after repeated wraparound");
}

int main(void)
{
    printf("========================================\n");
    printf(" CIRCULAR BUFFER TEST SUITE\n");
    printf("========================================\n");

    test_initialization();

    test_normal_push_pop();

    test_empty_buffer();

    test_full_buffer();

    test_overflow_policy();

    test_multiple_wraparounds();

    printf("\n========================================\n");
    printf(" TEST SUMMARY\n");
    printf("========================================\n");

    printf("Checks passed : %d\n", tests_passed);
    printf("Checks failed : %d\n", tests_failed);

    if (tests_failed == 0)
    {
        printf("\nALL BUFFER TESTS PASSED\n");
        return 0;
    }

    printf("\nSOME BUFFER TESTS FAILED\n");

    return 1;
}
