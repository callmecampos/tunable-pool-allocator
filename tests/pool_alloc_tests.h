/**
 * Tunable block pool allocator tests.
 *
 * Written by Felipe Campos, 11/15/2020.
 */

#ifndef POOL_ALLOC_TESTS_H
#define POOL_ALLOC_TESTS_H

#include <check.h>
#include <config.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/pool_alloc.h"

// =============== DEFINITIONS ===================

#ifdef linux
#define ck_assert_ptr_null(a) ((a) == NULL)
#define ck_assert_ptr_nonnull(a) ((a) != NULL)
#endif

size_t block_sizes[] = {1, 2, 3, 4, 5, 6, 7, 8,
                        9, 10, 11, 12, 13, 14, 15, 16,
                        17, 18, 19, 20, 21, 22, 23, 24, 25,
                        32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768};

// ============= HELPER FUNCTIONS =================

static uint8_t* fill_pool(size_t n)
{
    uint8_t* last_ptr = NULL;
    while (true)
    {
        uint8_t* ptr = (uint8_t*) pool_alloc(n);
        if (ptr == NULL)
        {
            break;
        }
        last_ptr = ptr;
    }

    return last_ptr;
}

int pool_size_bytes(size_t num_pools)
{
    return aligned((HEAP_SIZE_BYTES / num_pools) - sizeof(pool_header_t), sizeof(void*));
}

#endif /* POOL_ALLOC_TESTS_H */