/**
 * Tunable block pool allocator implementation.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pool_alloc.h"

static uint8_t g_pool_heap[65536];
static uint8_t *base_addr;
static int num_pools;
static int pool_size;
static int word_size;
static bool initialized = false;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

bool pool_init(const size_t *block_sizes, size_t block_size_count)
{
    if (initialized)
    {
        return false;
    }

    // Make sure we have a valid number of block sizes
    if (block_size_count <= 0 || block_size_count > MAX_NUM_POOLS)
    {
        return false;
    }

    // Initialize static global variables
    num_pools = (int)block_size_count;

    pool_size = (sizeof(g_pool_heap) / num_pools) - sizeof(pool_header_t);
    word_size = sizeof(void *);
    base_addr = g_pool_heap + (num_pools * sizeof(pool_header_t));

    // Populate the heap with pool headers, pools, and block headers inside of those pools
    for (int i = 0; i < num_pools; i++)
    {
        size_t block_size = aligned(block_sizes[i]); // byte alignment
        if (block_size == 0 || block_size > pool_size)
        {
            return false;
        }

        int num_blocks = pool_size / block_size;

        // Create a pool header for this block size and point it to the pool's first free block
        pool_header_t *hptr = (pool_header_t *)(g_pool_heap + (i * pool_size));
        hptr->size = (uint16_t)block_size;
        hptr->next_free = (block_header_t *)(base_addr + (i * pool_size));

        // Populate every block in the pool with a block header (this gets overwritten on allocation)
        block_header_t *last = NULL;
        for (int offset = 0; offset < num_blocks * block_size; offset += block_size)
        {
            block_header_t *bptr = (block_header_t *)(((uint8_t *)hptr->next_free) + offset);
            if (last != NULL)
            {
                last->next = bptr;
            }
            last = bptr;
        }
    }

    initialized = true;
    return true;
}

// TODO: each section (e.g. find free block) should probably be its own helper function

void *pool_alloc(size_t n)
{
    if (!initialized)
    {
        return NULL;
    }

    // Find the corresponding pool to allocate memory in
    pool_header_t *pool = find_pool_from_size(n);
    if (pool == NULL)
    {
        return NULL;
    }

    // Find an available free block in O(1) time
    block_header_t *free_block = pool->next_free;
    if (free_block == NULL)
    {
        // TODO: no free memory left [first check overflow external fragmentation space?] check higher pool header??
        // just increment pool_header pointer until you reach base_addr and return NULL if so
        // likely want this to be a subroutine, make code look clean, and *make sure to update relevant pool*
        return NULL;
    }

    // hmmm, this might cause problems if you leak between blocks, be careful.
    // maybe just don't update it if you're in a different block.
    pool->next_free = free_block->next;
    return (void *)free_block;
}

void pool_free(void *ptr)
{
    if (!initialized)
    {
        return;
    }

    pool_header_t *pool = find_pool_from_pointer(ptr);
    if (pool == NULL)
    {
        return;
    }

    printf("Successfully freed.\n"); // FIXME: have debugging flag (printf macro?)

    block_header_t *bptr = ptr;
    bptr->next = pool->next_free;
    pool->next_free = bptr;
}

// ============= HELPER FUNCTIONS =============

inline pool_header_t *get_pool(int i)
{
    return (pool_header_t *)(g_pool_heap + (i * pool_size));
    ;
}

pool_header_t *find_pool_from_size(size_t _n)
{
    size_t n = aligned(_n);

    int start = 0, end = num_pools - 1;
    int middle = (start + end) / 2;

    while (start < end)
    {
        pool_header_t *pool = get_pool(middle);
        if (pool->size < n)
        {
            start = middle + 1;
        }
        else if (pool->size == n)
        {
            return pool;
        }
        else
        {
            end = middle;
        }

        middle = (start + end) / 2;
    }

    printf("Failed to find pool from size %zu.", _n);
    return NULL;
}

pool_header_t *find_pool_from_pointer(void *ptr)
{
    // TODO: check if pointer is aligned
    int pool_index = (((uint8_t *)ptr) - base_addr) / pool_size; // TODO: check this
    if (pool_index >= 0 && pool_index < num_pools)
    {
        return get_pool(pool_index);
    }

    return NULL;
}

inline size_t aligned(size_t n)
{
    return BYTE_ALIGNMENT ? (n + word_size - 1) & ~(word_size - 1) : n;
}

// ============= DEBUG UTILS ==============

void memoryDump()
{
    return;
}

/**
 * TODO: handle external fragmentation by filling in with smaller blocks or shifting at initialization
 * Think about what kind of memory allocation needs the user might have. e.g. images. have a flag that allows different modes.
 * ^^basically you're handling wasted space at the end of each pool
 * total_wasted = heap_size - 16*3 - sum([pool_size // block_size for block_size in block_sizes])
 *
 * wasted(block_size) = pool_size % block_size --> use that for other memory sizes ooohhh (have that as a flag as it could be buggy)
 *
 * TODO: test cases: python script, simple C test cases, what edge cases are there? ...
 *
 * TODO: move typedefs and function headers into pool_alloc.h (see OpenWSN for example)
 */
