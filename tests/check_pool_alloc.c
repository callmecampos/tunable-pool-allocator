#include <config.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <check.h>

#include "../src/pool_alloc.h"

// ================ USEFUL FUNCTIONS ====================

int pool_size_bytes(size_t num_pools)
{
    return aligned((HEAP_SIZE_BYTES / num_pools) - sizeof(pool_header_t), 8);
}

// ================= INITIALIZATION =====================

/**
 * Basic initialization.
 */
START_TEST(basic_init)
{
    const size_t arr[] = {sizeof(int), 1024, 2048};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);
}
END_TEST

/**
 * Empty initialization.
 */
START_TEST(empty_init)
{
    const size_t arr[] = {};
    bool pool = pool_init(arr, 0);

    ck_assert(!pool);
}
END_TEST

/**
 * Initialization with block sizes that are too large for the heap.
 */
START_TEST(too_big_init)
{
    const size_t arr[] = {1024, 2048, 4096, 65536};
    bool pool = pool_init(arr, 4);

    ck_assert(!pool);
}
END_TEST

/**
 * Initialization with the most valid number of block sizes.
 */
START_TEST(max_init)
{
    size_t arr[248];
    for (int i = 0; i < 248; i++)
    {
        arr[i] = i + 1;
    }

    bool pool = pool_init(arr, 248);

    ck_assert(pool);
}

/**
 * Initialization with an invalid number of block sizes.
 */
START_TEST(exceeded_init)
{
    size_t arr[256];
    for (int i = 0; i < 256; i++)
    {
        arr[i] = i + 1;
    }

    bool pool = pool_init(arr, 256);

    ck_assert(!pool);
}

/**
 * Initialization with an ill-sorted array of block_sizes.
 */
START_TEST(unsorted_init)
{
    const size_t arr[] = {sizeof(int), sizeof(uint8_t), sizeof(uint16_t)};
    bool pool = pool_init(arr, 3);

    ck_assert(!pool);
}
END_TEST

/**
 * Initialization with a block_size_count not corresponding to the true number of block sizes.
 */
START_TEST(bad_size_input_init)
{
    const size_t arr[] = {sizeof(int), sizeof(uint8_t), sizeof(uint16_t)};
    bool pool = pool_init(arr, 50);

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
    tcase_add_test(tc_valid, max_init);
    suite_add_tcase(s, tc_valid);

    tc_failed = tcase_create("Failed initialization.");
    tcase_add_test(tc_failed, empty_init);
    tcase_add_test(tc_failed, too_big_init);
    tcase_add_test(tc_failed, exceeded_init);
    tcase_add_test(tc_failed, unsorted_init);
    tcase_add_test(tc_failed, bad_size_input_init);
    suite_add_tcase(s, tc_failed);

    return s;
}

// ================= ALLOCATION & FREEING =====================

/**
 * Checking that each of our allocated blocks in a given pool are equidistant by the right amount.
 */
START_TEST(alloc_check_relative_block)
{
    const size_t arr[] = {32, 64, 4096};
    size_t size = 3;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    uint8_t *last_ptr;
    for (int i = 0; i < size; i++)
    {
        last_ptr = (uint8_t *)pool_alloc(arr[i]);
        ck_assert_ptr_nonnull(last_ptr);
        for (int j = 0; j < pool_size / arr[i] - 1; j++)
        {
            uint8_t *new_ptr = (uint8_t *)pool_alloc(arr[i]);
            ck_assert_ptr_nonnull(new_ptr);
            ck_assert_msg(new_ptr - last_ptr == arr[i], "Expecting %zu found %ld", arr[i], new_ptr - last_ptr); // TODO: clean this up
            last_ptr = new_ptr;
        }
    }
}

/**
 * Checking that each of our allocated pools in the heap are equidistant on the heap by global pool size.
 */
START_TEST(alloc_check_relative_pool)
{
    const size_t arr[] = {32, 64, 1024, 4096};
    size_t size = 4;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    uint8_t *last_ptr = pool_alloc(arr[0]);
    ck_assert_ptr_nonnull(last_ptr);
    for (int i = 1; i < size; i++)
    {
        uint8_t *new_ptr = pool_alloc(arr[i]);
        ck_assert_ptr_nonnull(new_ptr);
        ck_assert_msg(new_ptr - last_ptr == pool_size, "Expecting %d found %ld", pool_size, new_ptr - last_ptr); // TODO: clean this up
        last_ptr = new_ptr;
    }
}

/**
 * Checking basic pointer allocation and value assignment to those pointers.
 */
START_TEST(alloc_value)
{
    const size_t arr[] = {sizeof(uint8_t), sizeof(uint16_t), sizeof(uint32_t)};
    bool pool = pool_init(arr, 3);
    ck_assert(pool);

    uint8_t *num8a = pool_alloc(sizeof(uint8_t));
    ck_assert_ptr_nonnull(num8a);
    *num8a = 10;

    uint8_t *num8b = pool_alloc(sizeof(uint8_t));
    ck_assert_ptr_nonnull(num8b);
    *num8b = 100;

    uint16_t *num16 = pool_alloc(sizeof(uint16_t));
    ck_assert_ptr_nonnull(num16);
    *num16 = 1000;

    uint32_t *num32 = pool_alloc(sizeof(uint32_t));
    ck_assert_ptr_nonnull(num32);
    *num32 = 10000;

    ck_assert(*num8a == 10 && *num8b == 100 && *num16 == 1000 && *num32 == 10000);
}
END_TEST

/**
 * Checking our allocation and freeing algorithm.
 * Each newly freed block should be first on the free list for that pool.
 */
START_TEST(alloc_and_free_mirror_short)
{
    const size_t arr[] = {sizeof(int), 1024, 2048};
    bool pool = pool_init(arr, 3);

    ck_assert(pool);

    int *p1 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p1);

    int *p2 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p2);

    pool_free(p1);
    pool_free(p2);

    int *p3 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p3);
    ck_assert_ptr_eq(p3, p2);

    int *p4 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p4);
    ck_assert_ptr_eq(p4, p1);

    pool_free(p3);
    pool_free(p4);
}
END_TEST

/**
 * Checking our allocation and freeing algorithm with more blocks.
 * Each newly freed block should be first on the free list for that pool.
 */
START_TEST(alloc_and_free_chain_long)
{
    const size_t arr[] = {sizeof(int)};
    bool pool = pool_init(arr, 1);

    ck_assert(pool);
    int pool_size = pool_size_bytes(1);

    int *p1 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p1);

    int *p2 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p2);

    int *p3 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p3);

    int *p4 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p4);

    int *p5 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p5);

    int *p6 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p6);

    pool_free(p1);
    pool_free(p4);
    pool_free(p6);
    pool_free(p3);
    pool_free(p2);
    pool_free(p5);

    int *p7 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p7);
    ck_assert_ptr_eq(p7, p5);

    int *p8 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p8);
    ck_assert_ptr_eq(p8, p2);

    int *p9 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p9);
    ck_assert_ptr_eq(p9, p3);

    int *p10 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p10);
    ck_assert_ptr_eq(p10, p6);

    int *p11 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p11);
    ck_assert_ptr_eq(p11, p4);

    int *p12 = pool_alloc(sizeof(int));
    ck_assert_ptr_nonnull(p12);
    ck_assert_ptr_eq(p11, p3);
}
END_TEST

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled.
 * 
 * Subsequent allocation attempts (when no other blocks are available) should
 * return NULL. Only after a pool_free() call can memory be allocated again.
 */
START_TEST(alloc_fill_pool_small_blocks)
{
    const size_t arr[] = {32};
    size_t size = 1;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Allocate all available 32-byte blocks except 1
    for (int i = 0; i < pool_size / arr[0] - 1; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(32));
    }

    // Allocate the last one
    uint8_t *last_ptr = pool_alloc(32);
    ck_assert_ptr_nonnull(last_ptr);

    // Can't allocate new memory there
    ck_assert_ptr_null(pool_alloc(32));

    pool_free(last_ptr);

    // Now we can allocate memory there since the last pointer was freed
    ck_assert_ptr_nonnull(pool_alloc(32));
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled.
 * 
 * Subsequent allocation attempts (when no other blocks are available) should
 * return NULL. Only after a pool_free() call can memory be allocated again.
 */
START_TEST(alloc_fill_pool_large_blocks)
{
    const size_t arr[] = {16384};
    size_t size = 1;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Allocate all available 16384-byte blocks except 1
    for (int i = 0; i < pool_size / arr[0] - 1; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(16384));
    }

    // Allocate the last one
    uint8_t *last_ptr = pool_alloc(16384);
    ck_assert_ptr_nonnull(last_ptr);

    // Can't allocate new memory there
    ck_assert_ptr_null(pool_alloc(16384));

    pool_free(last_ptr);

    // Now we can allocate memory there since the last pointer was freed
    ck_assert_ptr_nonnull(pool_alloc(16384));
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled,
 * and there lies more space in larger-block-size pools.
 * 
 * Subsequent allocation attempts (when no other blocks in the designated pool are available)
 * should allocate memory in the pool above it.
 */
START_TEST(alloc_overflow_pool_small)
{
    const size_t arr[] = {32, 64};
    size_t size = 2;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the pool for each block size
    for (int b = 0; b < size; b++)
    {
        for (int i = 0; i < pool_size / align(arr[b]); i++)
        {
            ck_assert_ptr_nonnull(pool_alloc(arr[0]));
        }
    }

    // Unable to allocate new memory for any size after all the pools are filled
    for (int b = 0; b < size; b++)
    {
        ck_assert_ptr_null(pool_alloc(arr[b]));
    }
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled,
 * and there lies more space in larger-block-size pools.
 * 
 * Subsequent allocation attempts (when no other blocks in the designated pool are available)
 * should allocate memory in the pool above it.
 */
START_TEST(alloc_overflow_pool_large)
{
    const size_t arr[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};
    size_t size = 10;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the pool for each block size
    for (int b = 0; b < size; b++)
    {
        for (int i = 0; i < pool_size / align(arr[b]); i++)
        {
            ck_assert_ptr_nonnull(pool_alloc(arr[0]));
        }
    }

    // Unable to allocate new memory for any size after all the pools are filled
    for (int b = 0; b < size; b++)
    {
        ck_assert_ptr_null(pool_alloc(arr[b]));
    }
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled,
 * and there lies more space in larger-block-size pools. This time for unaligned block sizes.
 * 
 * Subsequent allocation attempts (when no other blocks in the designated pool are available)
 * should allocate memory in the pool above it.
 */
START_TEST(alloc_overflow_pool_unaligned)
{
    const size_t arr[] = {3, 7, 13, 69, 86, 420, 1000};
    size_t size = 7;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the pool for each (unaligned) block size
    for (int b = 0; b < size; b++)
    {
        for (int i = 0; i < pool_size / align(arr[b]); i++)
        {
            ck_assert_ptr_nonnull(pool_alloc(arr[0]));
        }
    }

    // Unable to allocate new memory for any size after all the pools are filled
    for (int b = 0; b < size; b++)
    {
        ck_assert_ptr_null(pool_alloc(arr[b]));
    }
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled,
 * and there lies more space in larger-block-size pools. This time for unaligned block sizes.
 * 
 * Subsequent allocation attempts (when no other blocks in the designated pool are available)
 * should allocate memory in the pool above it.
 */
START_TEST(alloc_overflow_pool_mix)
{
    const size_t arr[] = {32, 64};
    size_t size = 2;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the 32-byte blocks
    for (int i = 0; i < pool_size / arr[0]; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(32));
    }

    // Fill up the 64-byte blocks except for 1
    for (int i = 0; i < pool_size / arr[1] - 1; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(64));
    }

    // Allocate the last one
    uint8_t *last_ptr = pool_alloc(64);
    ck_assert_ptr_nonnull(last_ptr);

    // Can't allocate smaller memory anywhere
    // Since the 32-byte and 64-byte pools are filled
    ck_assert_ptr_null(pool_alloc(32));

    // Release a block on the 64-byte pool
    pool_free(last_ptr);

    // Allocate 32-bytes on the 64-byte pool
    ck_assert_ptr_nonnull(pool_alloc(32));
}

/**
 * Checking our allocation and freeing algorithm once a pool is fully filled,
 * and there lies more space in larger-block-size pools. Ensuring that we don't
 * allocate space in smaller blocks for larger types.
 */
START_TEST(alloc_backwards_overflow)
{
    const size_t arr[] = {32, 64};
    size_t size = 2;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    int pool_size = pool_size_bytes(size);

    // Fill up the 64-byte blocks
    for (int i = 0; i < pool_size / arr[1]; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(64));
    }

    // Unsuccessfully allocate more 64-byte memory (it can't be allocated to the available 32-byte blocks)
    for (int i = 0; i < pool_size / arr[0]; i++)
    {
        ck_assert_ptr_null(pool_alloc(64));
    }

    // Successfully allocate the rest of the 32-byte blocks
    for (int i = 0; i < pool_size / arr[0]; i++)
    {
        ck_assert_ptr_nonnull(pool_alloc(32));
    }

    // No more memory to allocate
    ck_assert_ptr_null(pool_alloc(32));
}

/**
 * Checking our allocation and freeing algorithm for variable size allocations.
 * Also making sure behavior of size 4096 size blocks for a 2 pool allocation
 * is consistent i.e. you can fit 7 4096 size blocks in (2^16 / 2 - 16) bytes.
 * 
 * Anything more won't be allowed to allocate, however blocks size 32 and below
 * will fit nicely in the 32-byte block pool.
 * 
 */
START_TEST(alloc_varied_sizes)
{
    const size_t arr[] = {32, 4096};
    size_t size = 2;
    bool pool = pool_init(arr, size);

    ck_assert(pool);

    // Allocation to 32-byte blocks
    ck_assert_ptr_nonnull(pool_alloc(10));
    ck_assert_ptr_nonnull(pool_alloc(32));

    // Allocation to 4096-byte blocks
    ck_assert_ptr_nonnull(pool_alloc(4096)); // 1
    ck_assert_ptr_nonnull(pool_alloc(64));   // 2
    ck_assert_ptr_nonnull(pool_alloc(512));  // 3

    // Unsuccessful allocation of invalid sizes
    ck_assert_ptr_null(pool_alloc(8192));
    ck_assert_ptr_null(pool_alloc(4097));
    ck_assert_ptr_null(pool_alloc(0));

    // More allocation to a 4096-byte block
    ck_assert_ptr_nonnull(pool_alloc(4092)); // 4

    // Making sure I can allocate memory of any size and that freeing works
    for (int i = 1; i <= 4096; i++)
    {
        uint8_t *ptr = pool_alloc(i);
        ck_assert_ptr_nonnull(ptr);
        pool_free(ptr);
    }

    // Allocation of last available 4096-byte blocks
    ck_assert_ptr_nonnull(pool_alloc(512));  // 5
    ck_assert_ptr_nonnull(pool_alloc(512));  // 6
    ck_assert_ptr_nonnull(pool_alloc(3675)); // 7

    // None left
    ck_assert_ptr_null(pool_alloc(3675));

    // 32-byte blocks are still available
    ck_assert_ptr_nonnull(pool_alloc(16));
}

/**
 * Checking our allocation and freeing algorithm for a single pool with a single block paradigm.
 * 
 * Checking that we can allocate memory from 1 byte to (2^16 - 16) bytes inside that one block.
 */
START_TEST(alloc_all_sizes)
{
    const size_t arr[] = {pool_size_bytes(1)};
    size_t size = 1;
    bool pool = pool_init(arr, size);

    for (int i = 1; i < pool_size_bytes(1); i++)
    {
        uint8_t *ptr = pool_alloc(i);
        ck_assert_ptr_nonnull(ptr);
        pool_free(ptr);
    }
}

Suite *pool_alloc_suite(void)
{
    Suite *s;
    TCase *tc_valid;
    TCase *tc_overflow;
    TCase *tc_varying;

    s = suite_create("PoolAlloc");

    tc_valid = tcase_create("Valid allocation.");
    tcase_add_test(tc_valid, alloc_check_relative_block);
    tcase_add_test(tc_valid, alloc_check_relative_pool);
    tcase_add_test(tc_valid, alloc_value);
    tcase_add_test(tc_valid, alloc_and_free_mirror_short);
    suite_add_tcase(s, tc_valid);

    tc_overflow = tcase_create("Fill and overflow allocation.");
    tcase_add_test(tc_valid, alloc_fill_pool_small_blocks);
    tcase_add_test(tc_valid, alloc_fill_pool_large_blocks);
    tcase_add_test(tc_valid, alloc_overflow_pool_small);
    tcase_add_test(tc_valid, alloc_overflow_pool_large);
    tcase_add_test(tc_valid, alloc_overflow_pool_unaligned);
    tcase_add_test(tc_valid, alloc_overflow_pool_mix);
    tcase_add_test(tc_valid, alloc_backwards_overflow);
    suite_add_tcase(s, tc_overflow);

    tc_varying = tcase_create("Varying size allocation.");
    tcase_add_test(tc_varying, alloc_varied_sizes);
    tcase_add_test(tc_varying, alloc_all_sizes);
    suite_add_tcase(s, tc_varying);

    return s;
}

// =============== RUN TEST SUITES ================

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
