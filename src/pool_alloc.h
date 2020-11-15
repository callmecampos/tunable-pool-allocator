/**
 * Tunable block pool allocator header file.
 * 
 * Three assumptions about the user are made which influence the design of the block pool memory allocator.
 * 1. The user prefers smaller block size allocation and therefore allocates memory for such objects more often.
 * 2. Memory allocations of the similar type/size often happen repeatedly in succession.
 * 3. The user values minimized fragmentation and exceedingly low resource use (memory & computational footprint).
 * 
 * High level design decisions:
 * 1. The heap itself can and should be used to store state.
 * 2. The heap is subdivided evenly by number of pools, giving smaller objects more blocks to allocate into.
 *     a. If a smaller block pool is full, new allocations for said block size
 *        may take up blocks in the next non-empty pool of greater block size.
 *     b. There is no coalescing of smaller blocks since those take priority,
 *        though neither is there any splitting of larger blocks.
 * 3. At the beginning of the heap, we hold 16-byte headers identifying pool block size and pointing to first
 * available free block in that pool (NULL if no free blocks are available).
 * 4. There is no header/metadata overhead for allocated blocks, we can simply store a free list where each
 * free block holds a pointer to the next free block. This pointer is simply overwritten when the block is allocated,
 * and restored on pool_free().
 * 5. The memory allocator holds a pointer to the most recently used pool header.
 * 6. All headers, pools, and blocks are aligned in memory according to the size of memory addresses.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#ifndef POOL_ALLOC_H
#define POOL_ALLOC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// =================== DEFINITIONS =====================

#define MAX_NUM_POOLS 64
#define HEAP_SIZE_BYTES 65536

/**
 * Header struct occupying a freed block, pointing to the next
 * free block in the pool (may be NULL if at end of free list).
 * 
 * Note: 8 byte struct assuming 8-byte addressing. (4-byte on 32-bit, etc.)
 */
typedef struct block_header
{
    struct block_header* next;
} block_header_t;

/**
 * Header struct defining a pool size and pointing to
 * the next free block in that pool (NULL if none available).
 * 
 * Note: 16 byte struct assuming 8-byte addressing (8-byte on 32-bit, etc.)
 */
typedef struct pool_header
{
    size_t block_size;
    block_header_t* next_free;
} pool_header_t;

typedef uint8_t* byte_ptr_t;

// ============ TUNABLE BLOCK POOL ALLOCATOR ===============

/**
 * Initialize the pool allocator with a set of block sizes appropriate for this application.
 * Returns true on success, false on failure.
 *
 * Notes:
 * 1. This may only be called once per process.
 * 2. `block_size_count` doesn't exceed 64.
 * 3. `size_t* block_sizes` is pre-sorted and contains no duplicate elements.
 */
bool pool_init(const size_t* block_sizes, size_t block_size_count);

/**
 * Allocate n bytes.
 * Returns pointer to allocate memory on success, NULL pointer on failure.
 */
void* pool_alloc(size_t n);

/**
 * Release allocation pointed to by ptr.
 * 
 * Assumptions:
 * 1. Users are using pool_free() responsibly.
 *     i. pool_free() has undefined behavior when passed a pointer is not currently allocated by pool_alloc()
*/
void pool_free(void* ptr);

// ================ HELPER FUNCTIONS ==================

/**
 * Gets the pool header corresponding to the ith block size.
 */
static pool_header_t* get_pool(int i);

/**
 * Gets the block size index corresponding to the pool header.
 */
static int get_pool_index(pool_header_t* pool);

/**
 * Create a pool header for the ith block size and point it to the pool's first free block.
 */
static pool_header_t* create_pool_header(size_t block_size, int i);

/**
 * Populate every block in the given pool with a block header.
 * Returns the last block visited in the pool.
 */
static block_header_t* populate_block_headers(pool_header_t* pool);

/**
 * Binary search throuogh the pool headers to find the relevant pool.
 * 
 * Runs in O(log(N)) for N pools e.g. worst case 6 loops for 64 pools.
 */
static pool_header_t* find_pool_from_size(size_t n);

/**
 * Finds the pool header corresponding to the pointer in memory.
 * 
 * Returns NULL if an invalid pointer.
 */
static pool_header_t* find_pool_from_pointer(void* ptr);

/**
 * "Overloaded" aligned function, but specific to a pool allocator instance since it uses word_size.
 */
size_t align(size_t n);

/**
 * Returns a k-byte aligned size (4-bytes for 32-bit system, 8-bytes for 64-bit).
 * 
 * Example (64-bit system): aligned(4) = 8; aligned(16) = 16; aligned(18) = 24;
 */
size_t aligned(size_t n, size_t k);

// ================ DEBUG UTILS ==================

void memoryDump(uint8_t mask);

#endif /* POOL_ALLOC_H */
