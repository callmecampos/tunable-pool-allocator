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
static pool_header_t *last_accessed_pool;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

bool pool_init(const size_t *block_sizes, size_t block_size_count)
{
    // Make sure we have a valid number of block sizes
    if (initialized || block_sizes == NULL || block_size_count <= 0 || block_size_count > MAX_NUM_POOLS)
    {
        return false;
    }

    // Initialize static global variables
    num_pools = (int)block_size_count;

    pool_size = (sizeof(g_pool_heap) / num_pools) - sizeof(pool_header_t);
    word_size = sizeof(void *);
    base_addr = g_pool_heap + (num_pools * sizeof(pool_header_t));

    // printf("heap %p base %p\n", g_pool_heap, base_addr);

    // Populate the heap with pool headers, pools, and block headers inside of those pools
    size_t last_block_size = 0;
    for (int i = 0; i < num_pools; i++)
    {
        size_t block_size = block_sizes[i], aligned_block_size = aligned(block_size);
        if (block_size <= last_block_size || aligned_block_size == 0 || aligned_block_size > pool_size)
        {
            return false;
        }

        // Create a pool header for this block size and point it to the pool's first free block
        pool_header_t *pool = create_pool_header(block_size, i);

        // Populate every block in the pool with a block header (this gets overwritten on allocation)
        block_header_t *last_block = populate_block_headers(pool);

        if (FRAGMENT_OVERFLOW && i == 0) { // based on assumption that the smallest block size is the most used and will minimize unused memory
            populate_fragment_overflow(last_block, block_sizes, i);
        }

        // printf("BLOCK (%zu) ALIGNED (%zu) POOL (%p / %zu)\n", block_size, aligned_block_size, pool, pool->block_size);

        last_block_size = block_size;
    }

    last_accessed_pool = get_pool(num_pools-1);
    initialized = true;
    return true;
}

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

    // Allocate an available free block in O(1) time
    block_header_t *free_block = pool->next_free;

    // Update the pool's free block
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
        printf("oooof, null freed pointer.");
        return;
    }

    block_header_t *bptr = ptr;
    bptr->next = pool->next_free;
    pool->next_free = bptr;
}

// ============= HELPER FUNCTIONS =============

inline size_t aligned(size_t n)
{
    return (n + word_size - 1) & ~(word_size - 1);
}

inline pool_header_t *get_pool(int i)
{
    return (pool_header_t *)(g_pool_heap + (i * sizeof(pool_header_t)));
}

pool_header_t *create_pool_header(size_t block_size, int i)
{
    pool_header_t *pool = get_pool(i);
    pool->block_size = block_size;
    // pool->allocated = 1;
    pool->next_free = (block_header_t *)(base_addr + (i * pool_size));
    return pool;
}

block_header_t *populate_block_headers(pool_header_t *pool)
{
    block_header_t *last = NULL;
    for (size_t offset = 0; offset < pool_size; offset += aligned(pool->block_size))
    {
        block_header_t *bptr = (block_header_t *)(((uint8_t *)pool->next_free) + offset);
        if (last != NULL)
        {
            last->next = bptr;
        }
        last = bptr;
    }

    if (last != NULL)
    {
        last->next = NULL;
    }

    return last;
}

void populate_fragment_overflow(block_header_t *last_block, const size_t *block_sizes, int i) // this is clean, you continue populating then the overflow into higher memory handles itself. lazy approach should honestly handle itself, and should handle this better barring some edge cases of other ppl occupying. maybe restrict to you can just fuck around one ahead of you. still an edge case where n-1 and n-2 both can only occupy n. don't worry about it till you get there. true.
{
    if (i == num_pools - 1)
    {
        return;
    }

    size_t aligned_block_size = aligned(block_sizes[i]);
    int fragment_size = pool_size % aligned(block_sizes[i+1]);

    int j = 0;
    block_header_t *last = NULL;
    while (i + j < num_pools - 1)
    {
        j += 1;
        fragment_size = pool_size % aligned(block_sizes[i+j]);
        if (fragment_size < aligned_block_size)
        {
            continue;
        }

        printf("POPULATING POOL %zu with BLOCKS %zu\n\n", aligned(block_sizes[i+j]), aligned_block_size);

        // populate rest of fragment with opposite condition while loop (e.g. frag_size >= block_size, honestly just have it be a general if condition inside a while loop that just checks for j+i)
        for (size_t offset = 0; offset < pool_size; offset += aligned(block_sizes[i+j]))
        {
            block_header_t *bptr = (block_header_t *)((base_addr + ((i + j + 1) * pool_size) - fragment_size));
            if (last != NULL)
            {
                last->next = bptr;
            }
            last = bptr;
            printf("%d %p\n", j, bptr);
        }

        if (last != NULL)
        {
            last->next = NULL;
        }
    }
}

pool_header_t *find_pool_from_size(size_t n)
{
    int start = 0, end = num_pools - 1;
    int middle = (start + end) / 2;
    pool_header_t *pool = NULL;

    while (start <= end)
    {
        pool = get_pool(middle);
        // printf("%d %d %d %zu %zu\n", start, middle, end, pool->block_size, n);
        if (pool->block_size < n)
        {
            start = middle + 1;
        }
        else if (pool->block_size == n)
        {
            while (pool->next_free == NULL)
            {
                middle += 1;
                if (middle == num_pools)
                {
                    return NULL;
                }

                pool = get_pool(middle);
            }

            last_accessed_pool = pool;
            return pool;
        }
        else
        {
            end = middle - 1;
        }

        middle = (start + end) / 2;
    }

    printf("Failed to find pool from size %zu.\n", n);
    return NULL;
}

pool_header_t *find_pool_from_pointer(void *ptr)
{
    /** if ((uintptr_t) ptr % word_size != 0)
    {
        return NULL; // TODO: if not byte aligned, return null (basically just check modulo 8 yeah)? undefined behavior tho ya feel
    } */

    int pool_index = (((uint8_t *)ptr) - base_addr) / pool_size;
    if (pool_index >= 0 && pool_index < num_pools)
    {
        return get_pool(pool_index);
    }

    return NULL;
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
