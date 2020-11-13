/**
 * Tunable block pool allocator implementation.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "pool_alloc.h"

// 8 byte struct assuming 8-byte addressing
typedef struct block_header {
    struct block_header* next;
} block_header_t;

// 16 byte struct assuming 8-byte addressing
typedef struct {
    uint16_t size;
    block_header_t* next_free;
} pool_header_t;

#define MAX_BLOCK_SIZES 248

static uint8_t g_pool_heap[65536];
int pool_size;
int num_pools;
int word_size;
uint8_t* base_addr;
bool initialized = false; // TODO: check that this works as you expect? it honestly should lol

// ================ FUNCTION DECLARATIONS ==================

pool_header_t* find_pool_from_size(size_t n);
pool_header_t* find_pool_from_pointer(void* ptr);
size_t aligned(size_t n);

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

bool pool_init(const size_t* block_sizes, size_t block_size_count)
{
    if (block_size_count <= 0 || block_size_count > MAX_BLOCK_SIZES) {
        return false;
    }

    num_pools = (int) block_size_count;
    pool_size = sizeof(g_pool_heap) / num_pools - sizeof(pool_header_t);
    base_addr = g_pool_heap + (num_pools * sizeof(pool_header_t));
    word_size = (int) sizeof(void*);

    for (int i = 0; i < num_pools; i++) {
        size_t block_size = aligned(block_sizes[i]); // byte alignment
        if (block_size == 0 || block_size > pool_size) {
            initialized = false;
            return false;
        }

        int num_blocks = pool_size / block_size;

        pool_header_t *hptr = (pool_header_t*) (g_pool_heap + (i * pool_size));
        hptr->size = (uint16_t) block_size;
        hptr->next_free = (block_header_t*) (base_addr + (i * pool_size));

        // iterate through rest of blocks, updating their block headers
        // you may not need to do this since they're equidistant
        // could store the inverse, list of used blocks? might complicate things though
        block_header_t *last = NULL;
        for (int offset = 0; offset < num_blocks * block_size; offset += block_size) {
            block_header_t *bptr = (block_header_t*) (((uint8_t *) hptr->next_free) + offset);
            if (last != NULL) {
                last->next = bptr;
            }
            last = bptr;
        }
    }

    initialized = true;
    return true;
}

// TODO: each section (e.g. find free block) should probably be its own helper function

void* pool_alloc(size_t n)
{
    if (!initialized) {
        return NULL;
    }

    // find the corresponding pool to allocate memory in

    pool_header_t* pool = find_pool_from_size(n);
    if (pool == NULL) {
        printf("Hella fucked.\n");
        return NULL;
    }

    // find a free block

    block_header_t* free_block = pool->next_free;

    if (free_block == NULL) {
        // TODO: no free memory left, check higher pool header??
        // just increment pool_header pointer until you reach base_addr and return NULL if so
        // likely want this to be a subroutine, make code look clean
        return NULL;
    }

    // hmmm, this might cause problems if you leak between blocks, be careful.
    // maybe just don't update it if you're in a different block.
    pool->next_free = free_block->next;
    return (void *) free_block;
}

void pool_free(void* ptr)
{
    if (!initialized) {
        return;
    }

    // TODO: pointer arithmetic to figure out which pool you're in
    pool_header_t* pool = find_pool_from_pointer(ptr);
    if (pool == NULL) {
        return; // this should really never return null unless it's an invalid pointer
    }

    printf("Successfully freed.");

    block_header_t *bptr = ptr;
    bptr->next = pool->next_free;
    pool->next_free = bptr;
}

// ============= HELPER =============

pool_header_t* get_pool(int i) {
    pool_header_t *hptr = (pool_header_t*) (g_pool_heap + (i * pool_size));
    return hptr;
}

pool_header_t* find_pool_from_size(size_t _n) {
    uint16_t n = (uint16_t) aligned(_n);
    // binary search through headers to find start position: O(log(N)) runtime for N pools
    int start = 0, end = num_pools - 1;
    int middle = (start + end) / 2;

    printf("start %d end %d middle %d\n", start, end, middle);

    while (start < end) {
        pool_header_t *pool = get_pool(middle);
        printf("pool size %d n %d\n", pool->size, n);
        if (pool->size < n) {
            start = middle + 1;
        } else if (pool->size == n) {
            return pool;
        } else {
            end = middle;
        }

        middle = (start + end) / 2;
        printf("start %d end %d middle %d\n", start, end, middle);
    }

    return NULL; // TODO: implement me
}

pool_header_t* find_pool_from_pointer(void* ptr) {
    int pool_index = (((uint8_t*) ptr) - base_addr) / pool_size; // TODO: check this
    if (pool_index >= 0 && pool_index < num_pools) {
        return get_pool(pool_index);
    }

    return NULL;
}

// ============= UTILS ==============

inline size_t aligned(size_t n) {
    return n + (word_size - n) % word_size; // hm should probably do this bitwise, could quantify difference later lol
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
