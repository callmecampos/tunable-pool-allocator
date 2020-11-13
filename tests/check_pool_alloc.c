#include <config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <check.h>

#include "../src/pool_alloc.h"

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

Suite *pool_init_suite(void)
{
    Suite *s;
    TCase *tc_valid;
    TCase *tc_failed;

    s = suite_create("PoolInit");

    tc_valid = tcase_create("Valid initialization.");
    tcase_add_test(tc_valid, basic_init);
    suite_add_tcase(s, tc_valid);

    tc_failed = tcase_create("Failed initialization.");
    tcase_add_test(tc_failed, empty_init);
    tcase_add_test(tc_failed, too_big_init);
    suite_add_tcase(s, tc_failed);

    return s;
}

/**
START_TEST (unsorted_initialization)
{
    const size_t arr[] = {sizeof(int), sizeof(uint8_t), sizeof(uint16_t)};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);
}
END_TEST
*/

START_TEST(alloc)
{
    const size_t arr[] = {sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t)};
    bool pool = pool_init(arr, 3);

    int *num8a = pool_alloc(sizeof(uint8_t));
    *num8a = 10;

    int *num8b = pool_alloc(sizeof(uint8_t));
    *num8b = 100;

    int *num16 = pool_alloc(sizeof(uint16_t));
    *num16 = 1000;

    int *num32 = pool_alloc(sizeof(uint32_t));
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

Suite *pool_alloc_suite(void)
{
    Suite *s;
    TCase *tc_valid;
    TCase *tc_failed;

    s = suite_create("PoolAlloc");

    tc_valid = tcase_create("Valid allocation.");
    tcase_add_test(tc_valid, alloc);
    tcase_add_test(tc_valid, alloc_and_free);
    suite_add_tcase(s, tc_valid);

    // TODO: add some failed allocation tests

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
