/**
 * Tunable block pool allocator header file.
 * 
 * Assumptions:
 * 1. pool_init() can only be called once per process.
 * 2. Heap itself can and should be used to store state.
 * 3. Heap is subdivided evenly by number of pools, giving smaller objects more blocks to allocate into.
 * 4. The block sizes array is provided pre-sorted smallest to largest.
 *     i. This assumption accomodates O(log(N)) search for pool headers without having to sort the array during initialization.
 * 5. No duplicate block sizes are passed into the pool initialization function.
 * 6. No individual block size exceeds its respective allocated pool size.
 * 7. The number of pools doesn't exceed 248.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#ifndef POOL_ALLOC_H
#define POOL_ALLOC_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// =================== DEFINITIONS =====================

#define MAX_NUM_POOLS 248
#define HEAP_SIZE_BYTES 65536

/**
 * Header struct occupying a freed block, pointing to the next
 * free block in the pool (may be NULL if at end of free list).
 * 
 * 8 byte struct assuming 8-byte addressing.
 */
typedef struct block_header
{
    struct block_header *next;
} block_header_t;

/**
 * Header struct defining a pool size and pointing to
 * the next free block in that pool (NULL if none available).
 * 
 * Note: 16 byte struct assuming 8-byte addressing
 */
typedef struct pool_header
{
    size_t block_size;
    block_header_t *next_free; // don't need to store an entire 8 byte address, only need 2 and compute from base address
} pool_header_t;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

/**
 * Initialize the pool allocator with a set of block sizes appropriate for this application.
 * Returns true on success, false on failure.
 *
 */
bool pool_init(const size_t *block_sizes, size_t block_size_count);

/**
 * Allocate n bytes.
 * Returns pointer to allocate memory on success, NULL pointer on failure.
 */
void *pool_alloc(size_t n);

/**
 * Release allocation pointed to by ptr.
 * 
 * Assumptions:
 * 1. Users are using pool_free() responsibly.
 *     i. pool_free() has undefined behavior when passed a pointer is not currently allocated by pool_alloc()
*/
void pool_free(void *ptr);

// ================ HELPER FUNCTIONS ==================

/**
 * Gets the pool header corresponding to the ith block size.
 */
static pool_header_t *get_pool(int i);

/**
 * Gets the block size index corresponding to the pool header.
 */
static int get_pool_index(pool_header_t *pool);

/**
 * Create a pool header for the ith block size and point it to the pool's first free block.
 */
static pool_header_t *create_pool_header(size_t block_size, int i);

/**
 * Populate every block in the given pool with a block header.
 * Returns the last block visited in the pool.
 */
static block_header_t *populate_block_headers(pool_header_t *pool);

/**
 * Binary search throuogh the pool headers to find the relevant pool.
 * 
 * Runs in O(log(N)) for N pools e.g. worst case 8 loops for 256 pools.
 */
static pool_header_t *find_pool_from_size(size_t n);

/**
 * Finds the pool header corresponding to the pointer in memory.
 * 
 * Returns NULL if an invalid pointer.
 */
static pool_header_t *find_pool_from_pointer(void *ptr);

/**
 * "Overloaded" aligned function, but specific to a pool allocator instance since it uses word_size.
 */
size_t align(size_t n);

/**
 * Returns a k-byte aligned size (4-bytes for 32-bit system, 8-bytes for 64-bit).
 * 
 * Example (64-bit system): aligned(4) = 8; aligned(16) = 16; aligned(18) = 24;
 */
inline size_t aligned(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

#endif /* POOL_ALLOC_H */
