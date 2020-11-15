/**
 * Tunable block pool allocator initialization runtime tests.
 *
 * Written by Felipe Campos, 11/15/2020.
 */

#include <check.h>
#include <config.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pool_alloc_tests.h"
#include "../src/pool_alloc.h"

// ================= INITIALIZATION RUNTIME TESTS =====================

#define NUM_RUNS 1000

/**
 * Single pool initialization with byte-aligned block size.
 */
START_TEST(min_init)
{
    size_t arr[] = {sizeof(void*)};
    bool pool = pool_init(arr, 1);

    ck_assert(pool);
}
END_TEST

/**
 * Maximum number of pools initialization.
 */
START_TEST(max_init)
{
    size_t size = MAX_NUM_POOLS;
    size_t arr[size];
    for (int i = 0; i < size; i++)
    {
        arr[i] = i + 1;
    }

    bool pool = pool_init(arr, size);

    ck_assert(pool);
}
END_TEST

// ================ RUNTIME TEST SUITE DEFINITION ==================

Suite* pool_init_runtime_suite(void)
{
    Suite* s;
    TCase* tc;

    s = suite_create("PoolInitRuntime");

    tc = tcase_create("Initialization runtime.");
    tcase_add_loop_test(tc, min_init, 0, NUM_RUNS);
    tcase_add_loop_test(tc, max_init, 0, NUM_RUNS);
    suite_add_tcase(s, tc);

    return s;
}

// =============== RUN POOL INIT RUNTIME TESTS ================

int main(void)
{
    int number_failed;

    SRunner* sr = srunner_create(pool_init_runtime_suite());

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}