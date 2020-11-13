/**
 * Tunable block pool allocator implementation.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pool_alloc.h"

// 16 byte struct assuming 8-byte addressing
// maintains alignment when kept at start of heap or block (depending on design decision)
typedef struct {
    uint16_t size;
    block_header_t* first;
    block_header_t* free;
} pool_header_t;

typedef struct {
    bool free;
    block_header_t* next;
} block_header_t; // if we can arbitrarily use free and size as the same depending on context, may only need this and not pool_header

#define WORD_SIZE 8
static uint8_t g_pool_heap[65536];
int pool_size;
int num_pools;

// ================ FUNCTION DECLARATIONS ==================

size_t aligned(size_t n);

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    if (block_size_count == 0 || block_size_count >= sizeof(g_pool_heap)) {
        return false;
    }

    num_pools = (int) block_size_count;
    pool_size = sizeof(g_pool_heap) / num_pools - sizeof(pool_header_t);
    for (int i = 0; i < num_pools; i++) {
        size_t block_size = aligned(block_sizes[i]);
        if (block_size <= 0 || block_size > pool_size - sizeof(block_header_t) * pool_size / block_size) { // TODO: something like this lol
            return false;
        }

        pool_header_t *hptr = g_pool_heap + (i * pool_size);
        hptr->size = block_size;
        hptr->first = NULL; // first block in relevant pool --> quick address arithmetic
        hptr->free = hptr->first; // this might not be necessary

        // TODO: iterate through rest of blocks, updating their block headers, may not need to... since they're all equidistant

        printf("CHECK: (%p) (%d) (%p)\n", hptr, hptr->size, &g_pool_heap[i * pool_size]);
    }

    return true; // TODO: Implement me!
}

void* pool_alloc(size_t n)
{
    pool_header_t* pool = find_pool_header(n);
    if (pool == NULL) {
        return NULL;
    }

    if (pool->free == NULL) {
        // no free memory left, check higher pool header??
    }

    void* ptr = g_pool_heap + num_pools * pool_size + n + sizeof(block_header_t); // FIXME: filler
    return ptr; // TODO: Implement me!
}

void pool_free(void* ptr)
{
    // deallocate and update free pointer, have next point to previous position of malloc
    // TODO: Implement me!
}

// ============= UTILS ==============

inline size_t aligned(size_t n) {
    return n + (WORD_SIZE - n) % WORD_SIZE; // hm should probably do this bitwise
}
