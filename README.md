# Tunable Block Pool Allocator

## How to compile and run: (on Unix-based OS)

Dependency installation:

macOS:
```
brew install check
brew install automake
```

Linux:
```
sudo apt-get install check
sudo apt-get install autoconf
```

Using Autotools (recommended):
```
autoreconf --install
./configure
make
make check
cat tests/test-suite.log # view test logs
```

Using CMake:
```
cmake .
make
make test
```

---

## High-level implementation

![High-level implementation diagram from Notability.](./tunable-block-pool-allocator-diagram.png "High-level implementation diagram from Notability.")

### Assumptions:
Three assumptions about the user are made which influence the design of this block pool memory allocator.
1. The user prefers smaller block size allocation and therefore allocates memory for such objects more often.
2. Memory allocations of the similar type/size often happen repeatedly in succession.
3. The user primarily values exceedingly low resource use (i.e. memory & computational footprint) followed by minimized fragmentation.

### Design decisions and tradeoffs:
1. The heap itself can and should be used to store state.
    1. Pool headers store block size and a pointer to the head block of the free list in that pool (NULL if none are free). Block headers make up the free lists in their respective pools, and simply store an address pointing to the next free block within the same pool.
    1. **Tradeoff:** Storing state in the heap ensures simplicity of the implementation. We do sacrifice a small memory footprint by storing pool headers at the start of the heap, though it's negligible relative to the entire heap (e.g. for 64 pools, only ~1% of the total heap is used for pool headers).
    1. Block headers, on the other hand, incur no additional memory footprint since we don't have to store the size of the allocated block in the header, and can store the header within the free block itself, allowing that memory to be overwritten on allocation.
1. The heap is subdivided evenly by number of pools, giving smaller objects more blocks to allocate into.
    1. Additionally, if a smaller block pool is full, new allocations for said block size may take up blocks in the next non-empty pool of greater block size. There is no coalescing of smaller blocks since those take priority, though neither is there any splitting of larger blocks.
    1. **Tradeoff:** The tradeoff for the above decisions is embedded in the user's preference for smaller block size allocation. Another tradeoff is that the latter decision increases internal fragmentation, and though we could split large blocks into smaller sub-blocks to mitigate this, that would incur an unnecessary computational and memory footprint.
1. The memory allocator holds a pointer to the most recently used pool header.
    1. **Tradeoff:** There is no real tradeoff here since there is a negligible memory footprint for this. This is based off the assumption that memory allocations of similar size often happen repeatedly in succession, meaning we don't always have to perform an O(log(b)) search through pool headers to find the relevant pool on pool_alloc() call, instead maintaining a reference to the most recently used pool, providing us constant time access when used in succession.
1. All headers, pools, and blocks are aligned in memory according to the size of memory addresses.
    1. e.g. 2-byte aligned for a 16-bit processor, 4-byte aligned for a 32-bit processor, 8-byte aligned for a 64-bit processor, even 3-byte aligned on a 24-bit processor (if you can find one!).
    1. **Tradeoff:** We want to make sure memory accesses are as efficient as possible, so are willing to tradeoff some internal fragmentation in exchange for efficiency by respecting the target CPU's memory access patterns. Additionally, this ensures that pools with block sizes smaller than memory address sizes can still hold block headers (which hold an address to the next free block in that pool) in each unallocated block.
1. `pool_free()` has undefined behavior when passed a pointer is not currently allocated by pool_alloc() (whether because it wasn't allocated in the first place or it was already freed).
    1. **Tradeoff:** Though we have the ability to detect unaligned pointers and invalid free calls, we chose to keep in line with how classical free functions operate to minimize computational and memory footprint to keep pool_free() a constant time operation.
1. If the allocator cannot accomodate all pool sizes evenly divided among the heap during `pool_init()`, it will return false.
1. `pool_init()` may only be called once per a process.

### Valid inputs:
1. The block sizes array is passed in pre-sorted smallest to largest, has no duplicate elements, and its length does not exceed 64.
    1. This places burden on the user to provide a specifically formatted block size list. However, this assumption was made to accomodate O(log(b)) search for pool headers on an allocation request without having to sort the block size array during initialization, which would incur an additional O(b * log(b)) computational cost for initialization.
    1. The even subdivision of the heap among pools in addition to the accumulated byte alignment makes for strange circumstances at larger pool numbers where, due to alignment, there lies no additional space for later pools to allocate even a single block within the heap without overflowing. So, to simplify, we chose to make 64 blocks the maximum amount.
    1. The largest block size count at which a valid set of block sizes can exist is therefore {1, 2, 3, 4, 5, ..., 62, 63, 64} which, 8-byte aligned, becomes {8, 8, 8, 8, 8, ..., 64, 64, 64}).

## Time and Space Complexity Analysis

```
b = number of pools (up to 64)
N = total number of blocks (up to 8190)
```

<ins>`bool pool_init(const size_t* block_sizes, size_t block_size_count)`</ins>

**Time:**

Normal init: O(b + N)

*With lazy init (not implemented): O(b)*

**Space:**

Normal: O(b + N)

*With lazy init (not implemented): O(b)*

<ins>`void* pool_alloc(size_t n)`</ins>

**Time:**

Best Case (Cache Hit): O(1)

Average Case (Binary Search): O(log(b))

Worst Case (All Pools Full): O(b)

**Space:**

Best Case (Cache Hit): O(1)

Average Case: O(b)

<ins>`void pool_free(void* ptr)`</ins>

**Time:** O(1)

**Space:** O(1)

### Additional Future Optimizations
1. Populate block headers lazily as memory becomes allocated rather than all at once during initialization.
    1. This would lower initialization complexity to O(b) in both time and space. (branch `lazy`)
1. External fragmentation between pools may be used to hold smaller size objects (e.g. leftover "dead" space after a 1024-byte pool before a subsequent 2048-byte pool may be split up into several 32-byte blocks).
    1. Filling leftover external fragmentation exclusively with the smallest memory block size leads to the greatest minimization of external fragmentation while also keeping in line with the assumption that small block size allocations are more desirable. (branch `fragment`)
1. Compact all the pools together, combining all the leftover space at the end of the heap.
    1. This would get completely rid of external fragmentation, leaving one contiguous accumulated block of memory at the end of the heap to use however one wants. We initially chose not to do this to simplify pointer arithmetic for indexing into pools and their respective blocks. (branch `compact`)

