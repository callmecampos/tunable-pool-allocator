#include <config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <check.h>

#include "../src/pool_alloc.h"

// ================= INITIALIZATION =====================

START_TEST(basic_init)
{
    const size_t arr[] = {sizeof(int), 1024, 2048};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);
}
END_TEST

START_TEST(empty_init)
{
    const size_t arr[] = {};
    bool pool = pool_init(arr, 0);

    ck_assert(!pool);
}
END_TEST

START_TEST(too_big_init)
{
    const size_t arr[] = {1024, 2048, 4096, 65536};
    bool pool = pool_init(arr, 4);

    ck_assert(!pool);
}
END_TEST

START_TEST(max_init)
{
    size_t arr[248];
    for (int i = 0; i < 248; i++) {
        arr[i] = i+1;
    }

    bool pool = pool_init(arr, 248);

    ck_assert(pool);
}

START_TEST(exceeded_init)
{
    size_t arr[256];
    for (int i = 0; i < 256; i++) {
        arr[i] = i+1;
    }

    bool pool = pool_init(arr, 256);

    ck_assert(!pool);
}

Suite *pool_init_suite(void)
{
    Suite *s;
    TCase *tc_valid;
    TCase *tc_failed;

    s = suite_create("PoolInit");

    tc_valid = tcase_create("Valid initialization.");
    tcase_add_test(tc_valid, basic_init);
    tcase_add_test(tc_valid, max_init);
    suite_add_tcase(s, tc_valid);

    tc_failed = tcase_create("Failed initialization.");
    tcase_add_test(tc_failed, empty_init);
    tcase_add_test(tc_failed, too_big_init);
    tcase_add_test(tc_failed, exceeded_init);
    suite_add_tcase(s, tc_failed);

    return s;
}

START_TEST (unsorted_initialization)
{
    const size_t arr[] = {sizeof(int), sizeof(uint8_t), sizeof(uint16_t)};
    bool pool = pool_init(arr, 3);

    ck_assert(!pool);
}
END_TEST

// ================= ALLOCATION & FREEING =====================

START_TEST(alloc_check_relative_block)
{
    const size_t arr[] = {32, 64, 4096};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);

    uint8_t *last_ptr;
    for (int i = 0; i < 3; i++)
    {
        last_ptr = (uint8_t *) pool_alloc(arr[i]);
        ck_assert(last_ptr != NULL);
        for (int j = 0; j < 3; j++)
        {
            uint8_t *new_ptr = (uint8_t *) pool_alloc(arr[i]);
            ck_assert_msg(new_ptr - last_ptr == arr[i], "Expecting %zu found %ld", arr[i], new_ptr - last_ptr);
            last_ptr = new_ptr;
        }
    }
}

START_TEST(alloc_check_relative_pool)
{
    const size_t arr[] = {32, 64, 1024, 4096};
    bool pool = pool_init(arr, 4);

    ck_assert(pool);

    int pool_size = (65536 / 4) - 16;

    uint8_t *last_ptr = pool_alloc(arr[0]);
    for (int i = 1; i < 4; i++) {
        uint8_t *new_ptr = pool_alloc(arr[i]);
        ck_assert_msg(new_ptr - last_ptr == pool_size, "Expecting %d found %ld", pool_size, new_ptr - last_ptr);
        last_ptr = new_ptr;
    }
}

START_TEST(alloc_value)
{
    const size_t arr[] = {sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t)};
    bool pool = pool_init(arr, 3);
    ck_assert(pool);

    uint8_t *num8a = pool_alloc(sizeof(uint8_t));
    ck_assert(num8a != NULL);
    *num8a = 10;

    uint8_t *num8b = pool_alloc(sizeof(uint8_t));
    ck_assert(num8b != NULL);
    *num8b = 100;

    uint16_t *num16 = pool_alloc(sizeof(uint16_t));
    ck_assert(num16 != NULL);
    *num16 = 1000;

    uint32_t *num32 = pool_alloc(sizeof(uint32_t));
    ck_assert(num32 != NULL);
    *num32 = 10000;

    ck_assert(*num8a == 10 && *num8b == 100 && *num16 == 1000 && *num32 == 10000);
}
END_TEST

START_TEST(alloc_and_free)
{
    const size_t arr[] = {sizeof(int), 1024, 2048};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);

    int *p1 = pool_alloc(sizeof(int));
    ck_assert(p1 != NULL);

    int *p2 = pool_alloc(sizeof(int));
    ck_assert(p2 != NULL);

    pool_free(p1);
    pool_free(p2);

    int *p3 = pool_alloc(sizeof(int));
    ck_assert(p3 != NULL && p3 == p2);

    int *p4 = pool_alloc(sizeof(int));
    ck_assert(p4 != NULL && p4 == p1);

    pool_free(p3);
    pool_free(p4);
}
END_TEST

START_TEST(alloc_fill_unfill_blocks_32)
{
    const size_t arr[] = {32};
    bool pool = pool_init(arr, 1);

    ck_assert(pool);

    for (int i = 0; i < (65536 - 16) / 32; i++) {
        ck_assert(pool_alloc(32) != NULL);
    }

    uint8_t *last_ptr = pool_alloc(32);
    ck_assert(pool_alloc(32) == NULL);

    pool_free(last_ptr);

    ck_assert(pool_alloc(32) != NULL);
}

START_TEST(alloc_fill_unfill_blocks_4096)
{
    const size_t arr[] = {4096};
    bool pool = pool_init(arr, 1);

    ck_assert(pool);

    for (int i = 0; i < (65536 - 16) / 4096; i++) {
        ck_assert(pool_alloc(4096) != NULL);
    }

    uint8_t *last_ptr = pool_alloc(4096);
    ck_assert(pool_alloc(4096) == NULL);

    pool_free(last_ptr);

    ck_assert(pool_alloc(4096) != NULL);
}

START_TEST(alloc_overflow_pool)
{
    const size_t arr[] = {32, 4096};
    bool pool = pool_init(arr, 2);

    ck_assert(pool);

    for (int i = 0; i < ((65536 / 2) - 16) / 32 + 1; i++) {
        ck_assert(pool_alloc(32) != NULL);
    }

    for (int i = 0; i < ((65536 / 2) - 16) / 4096; i++) {
        ck_assert(pool_alloc(32) != NULL);
    }

    uint8_t *last_ptr = pool_alloc(4096);
    ck_assert(last_ptr != NULL);
    ck_assert(pool_alloc(32) == NULL);
    ck_assert(pool_alloc(4096) == NULL);
}

START_TEST(alloc_free_overflow_pool_fragment)
{
    const size_t arr[] = {32, 4096};
    bool pool = pool_init(arr, 2);

    ck_assert(pool);

    for (int i = 0; i < ((65536 / 2) - 16) / 32 + 1; i++) {
        ck_assert(pool_alloc(32) != NULL);
    }

    for (int i = 0; i < ((65536 / 2) - 16) / 4096; i++) {
        ck_assert(pool_alloc(4096) != NULL);
    }

    uint8_t *last_ptr = pool_alloc(4096);
    ck_assert(last_ptr != NULL);
    ck_assert(pool_alloc(32) == NULL);

    pool_free(last_ptr);

    ck_assert(pool_alloc(32) != NULL);
}

START_TEST(alloc_overflow_fragment)
{
    // TODO: implement me with for loops
    const size_t arr[] = {32, 4096};
    bool pool = pool_init(arr, 2);

    ck_assert(pool);

    // fill up 4096 all the way, then 32 all the way, then keep going with 32
}

START_TEST(alloc_invalid_size)
{
    // TODO: implement me
    const size_t arr[] = {32, 4096};
    bool pool = pool_init(arr, 2);

    ck_assert(pool);

    ck_assert(pool_alloc(32) != NULL);
    ck_assert(pool_alloc(4096) != NULL);
    ck_assert(pool_alloc(10) == NULL);
}

Suite *pool_alloc_suite(void)
{
    Suite *s;
    TCase *tc_valid;
    TCase *tc_overflow;
    TCase *tc_failed;

    s = suite_create("PoolAlloc");

    tc_valid = tcase_create("Valid allocation.");
    tcase_add_test(tc_valid, alloc_check_relative_block);
    tcase_add_test(tc_valid, alloc_check_relative_pool);
    tcase_add_test(tc_valid, alloc_value);
    tcase_add_test(tc_valid, alloc_and_free);
    suite_add_tcase(s, tc_valid);

    tc_overflow = tcase_create("Fill and overflow allocation.");
    tcase_add_test(tc_valid, alloc_fill_unfill_blocks_32);
    tcase_add_test(tc_valid, alloc_fill_unfill_blocks_4096);
    tcase_add_test(tc_valid, alloc_overflow_pool);
    tcase_add_test(tc_valid, alloc_free_overflow_pool_fragment);
    tcase_add_test(tc_valid, alloc_overflow_fragment);
    suite_add_tcase(s, tc_overflow);

    tc_failed = tcase_create("Failed allocation.");
    tcase_add_test(tc_valid, alloc_invalid_size);
    suite_add_tcase(s, tc_failed);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = pool_init_suite();
    sr = srunner_create(s);
    srunner_add_suite(sr, pool_alloc_suite());

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
