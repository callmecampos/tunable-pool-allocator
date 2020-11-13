/**
 * Tunable block pool allocator header file.
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
#define BYTE_ALIGNMENT true

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
    size_t size;
    block_header_t *next_free;
} pool_header_t;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

/**
 * Initialize the pool allocator with a set of block sizes appropriate for this application.
 * Returns true on success, false on failure.
 */
bool pool_init(const size_t *block_sizes, size_t block_size_count);

/**
 * Allocate n bytes.
 * Returns pointer to allocate memory on success, NULL pointer on failure.
 */
void *pool_alloc(size_t n);

/**
 * Release allocation pointed to by ptr.
*/
void pool_free(void *ptr);

// ================ HELPER FUNCTIONS ==================

/**
 * Gets the header corresponding to the ith pool size.
 */
pool_header_t *get_pool(int i);

/**
 * Binary search throuogh the pool headers to find the relevant pool.
 * 
 * Runs in O(log(N)) for N pools e.g. worst case 8 loops for 256 pools.
 */
pool_header_t *find_pool_from_size(size_t n);

/**
 * Finds the pool header corresponding to the pointer in memory.
 * 
 * Returns NULL if an invalid pointer.
 */
pool_header_t *find_pool_from_pointer(void *ptr);

/**
 * If BYTE_ALIGNMENT is true, returns a k-byte aligned size (4-bytes for 32-bit system, 8-bytes for 64-bit).
 * 
 * Example (64-bit system): aligned(4) = 8; aligned(16) = 16; aligned(18) = 24;
 */
size_t aligned(size_t n);

#endif /* POOL_ALLOC_H */
