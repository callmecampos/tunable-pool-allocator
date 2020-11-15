/**
 * Tunable block pool allocator runtime tests.
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

// ================= ALLOCATION RUNTIME TESTS =====================

#define NUM_RUNS 1000

/**
 * Checking our allocation algorithm for various pools.
 * 
 * Subsequent allocation attempts (when no other blocks in the designated pool are available)
 * should allocate memory in the pool above it.
 */
START_TEST(alloc_pool_runtime_check)
{
    size_t size = MAX_NUM_POOLS;
    size_t arr[size];
    for (int i = 0; i < size; i++)
    {
        arr[i] = i + 1;
    }

    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the pool for each block size
    // (backwards so we don't populate larger blocks with smaller ones)
    for (int b = size - 1; b >= 0; b--)
    {
        // Allocate all available blocks
        uint8_t* last_alloc = fill_pool(arr[b]);
        ck_assert_ptr_nonnull(last_alloc);

        // Can't allocate new memory there
        ck_assert_ptr_null(pool_alloc(arr[b]));

        pool_free(last_alloc);

        // Now we can allocate memory there since the last pointer was freed
        ck_assert_ptr_nonnull(pool_alloc(arr[b]));
    }

    // Unable to allocate new memory for any size after all the pools are filled
    for (int b = 0; b < size; b++)
    {
        ck_assert_ptr_null(pool_alloc(arr[b]));
    }
}
END_TEST

// ================ RUNTIME TEST SUITE DEFINITION ==================

Suite* pool_alloc_runtime_suite(void)
{
    Suite* s;
    TCase* tc;

    s = suite_create("PoolAllocRuntime");

    tc = tcase_create("Allocation runtime.");
    tcase_add_loop_test(tc, alloc_pool_runtime_check, 0, NUM_RUNS);
    suite_add_tcase(s, tc);

    return s;
}

// =============== RUN POOL ALLOC RUNTIME TESTS ================

int main(void)
{
    int number_failed;

    SRunner* sr = srunner_create(pool_alloc_runtime_suite());

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}