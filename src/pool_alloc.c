/**
 * Tunable block pool allocator implementation.
 *
 * Written by Felipe Campos, 11/12/2020.
 */

#include "pool_alloc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t g_pool_heap[65536];

static uint8_t *base_addr;
static uint8_t *end_addr;
static int num_pools;
static int pool_size;
static int byte_align;
static bool initialized = false;
static pool_header_t *last_used_pool;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

bool pool_init(const size_t *block_sizes, size_t block_size_count)
{
    // Make sure we have a valid number of block sizes
    if (initialized ||
        block_sizes == NULL || block_size_count <= 0 || block_size_count > MAX_NUM_POOLS)
    {
        return false;
    }

    // Initialize static global variables
    byte_align = sizeof(void *);
    num_pools = (int)block_size_count;
    pool_size = align((sizeof(g_pool_heap) / num_pools) - sizeof(pool_header_t));
    base_addr = g_pool_heap + (num_pools * sizeof(pool_header_t));
    end_addr = g_pool_heap + HEAP_SIZE_BYTES;

    // Populate the heap with pool headers and pools of blocks
    size_t last_block_size = 0;
    for (int i = 0; i < num_pools; i++)
    {
        size_t block_size = block_sizes[i];
        if (block_size <= last_block_size || block_size == 0 ||
            align(block_size) > pool_size)
        {
            return false;
        }

        // Create a pool header for this block size
        // and point it to the pool's first free block
        pool_header_t *pool = create_pool_header(block_size, i);
        if (pool == NULL)
        {
            return false;
        }

        // Populate every free block in the pool with a block header
        // (which get overwritten on allocation)
        populate_block_headers(pool);

        last_block_size = block_size;
    }

    last_used_pool = get_pool(0);
    initialized = true;

    return true;
}

void *pool_alloc(size_t n)
{
    if (!initialized || n == 0)
    {
        return NULL;
    }

    // Find the corresponding pool to allocate memory
    pool_header_t *pool = find_pool_from_size(n);
    if (pool == NULL)
    {
        return NULL;
    }

    // Pop off an available free block in O(1) time
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
        return;
    }

    block_header_t *bptr = ptr;
    bptr->next = pool->next_free;
    pool->next_free = bptr;
}

// ============= HELPER FUNCTIONS =============

static pool_header_t *create_pool_header(size_t block_size, int i)
{
    pool_header_t *pool = get_pool(i);
    pool->block_size = block_size;

    byte_ptr_t first_free = base_addr + (i * pool_size);
    int num_blocks = pool_size / (align(block_size));
    if (first_free + align(block_size) > end_addr)
    {
        return NULL;
    }

    pool->next_free = (block_header_t *)first_free;

    return pool;
}

static block_header_t *populate_block_headers(pool_header_t *pool)
{
    size_t aligned_block_size = align(pool->block_size);
    byte_ptr_t first_free = (byte_ptr_t)pool->next_free;

    block_header_t *last = NULL;
    for (size_t offset = 0;
         offset + aligned_block_size <= pool_size;
         offset += aligned_block_size)
    {
        if (first_free + offset + aligned_block_size > end_addr)
        {
            continue;
        }

        block_header_t *bptr = (block_header_t *)(first_free + offset);
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

static pool_header_t *find_pool_from_size(size_t n)
{
    int start = 0, end = num_pools - 1;
    int middle = (start + end) / 2;
    pool_header_t *pool = NULL;

    // Check cache for last used pool
    if (n == last_used_pool->block_size)
    {
        pool = last_used_pool;
        middle = get_pool_index(pool);
    }
    // else binary search through pool headers
    else
    {
        while (start <= end)
        {
            pool = get_pool(middle);
            if (pool->block_size < n)
            {
                start = middle + 1;
            }
            else if (pool->block_size == n)
            {
                break;
            }
            else
            {
                end = middle - 1;
            }

            middle = (start + end) / 2;
        }
    }

    // Check out larger block size pools if the current has no free space
    pool = get_pool(middle);
    while (n > pool->block_size || pool->next_free == NULL)
    {
        middle += 1;
        if (middle == num_pools)
        {
            return NULL;
        }

        pool = get_pool(middle);
    }

    last_used_pool = pool;
    return pool;
}

static pool_header_t *find_pool_from_pointer(void *ptr)
{
    int pool_index = (((byte_ptr_t)ptr) - base_addr) / pool_size;
    if (pool_index >= 0 && pool_index < num_pools)
    {
        return get_pool(pool_index);
    }

    return NULL;
}

static inline pool_header_t *get_pool(int i)
{
    return (pool_header_t *)(g_pool_heap + (i * sizeof(pool_header_t)));
}

static int get_pool_index(pool_header_t *pool)
{
    return ((uintptr_t)(((byte_ptr_t)pool) - g_pool_heap)) / sizeof(pool_header_t);
}

inline size_t align(size_t n) { return aligned(n, byte_align); }

inline size_t aligned(size_t n, size_t k) { return (n + k - 1) & ~(k - 1); }

// ============= DEBUG UTILS ==============

void memoryDump(uint8_t mask)
{
    if (!initialized)
    {
        printf("Warning: Allocator not initialized.\n");
    }

    if (mask & 0b1)
    {
        printf("---------- Init Information ----------\n\n");

        printf("Byte Alignment: %d\nNumber of Pools: %d\nPool Size (Bytes): %d\n\n",
               byte_align, num_pools, pool_size);

        printf("[Heap]\nStart: %p\nBase: %p\nEnd: %p\n\n",
               g_pool_heap, base_addr, end_addr);
    }

    if (mask & 0b10)
    {
        printf("------------ Pool Headers ------------\n\n");

        for (int i = 0; i < num_pools; i++)
        {
            pool_header_t *pool = get_pool(i);
            printf("[Pool %d]\nBlock Size (Aligned): %zu (%zu)\nNumber of Blocks: %ld\nNext Free: %p\n\n",
                   i, pool->block_size, align(pool->block_size), pool_size / align(pool->block_size), pool->next_free);
        }
    }

    if (mask & 0b100)
    {
        printf("---------- Other Information ----------\n\n");
        printf("Last Used Pool: [Pool %d]\nBlock Size: %zu\nNext Free: %p\n\n",
               get_pool_index(last_used_pool), last_used_pool->block_size, last_used_pool->next_free);
    }

    return;
}
