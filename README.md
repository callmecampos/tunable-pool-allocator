# Zipline Take-Home Challenge: Tunable Block Pool Allocator

## How to compile and run: (on Unix-based OS, see tests/Makefiles for Windows-specific instructions)

Using Autotools:
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
    1. Tradeoff: This ensures simplicity and elegance in the implementation. We do sacrifice a small memory footprint in the process, though it's negligible (~6% of the heap size in the absolute worst case).
1. The heap is subdivided evenly by number of pools, giving smaller objects more blocks to allocate into. Additionally, if a smaller block pool is full, new allocations for said block size may take up blocks in the next non-empty pool of greater block size.
    1. Tradeoff: The tradeoff for these two logistical decisions is embedded in the user's preference for smaller block size allocation. Another tradeoff is that the latter decision increases internal fragmentation, and though we could split large blocks into smaller sub-blocks to mitigate this, that would incur an unnecessary computational and memory footprint.
1. The memory allocator holds a pointer to the most recently used pool header.
    1. Tradeoff: There is no real tradeoff here since there is a negligible memory footprint for this. This is based off the assumption that memory allocations of similar size often happen repeatedly in succession, meaning we don't always have to perform an O(log(b)) search through pool headers to find the relevant pool on pool_alloc() call, instead maintaining a reference to the most recently used pool, providing us constant time access when used in succession.
1. All headers, pools, and blocks are 8-byte aligned (for a 64-bit processor).
    1. Tradeoff: Want to make sure memory accesses are as efficient as possible, so are willing to tradeoff some internal fragmentation caused by the occasional unaligned allocation size in exchange for efficiency.
1. pool_free() has undefined behavior when passed a pointer is not currently allocated by pool_alloc() (whether because it wasn't allocated in the first place or it was already freed).
    1. Tradeoff: Though we have the ability to detect unaligned pointers and invalid free calls, we chose to keep in line with how classical free functions operate to minimize computational and memory footprint to keep pool_free() a constant time operation.
1. pool_init() is only called once per a process.

### Valid inputs:
1. The block sizes array is passed in pre-sorted smallest to largest, has no duplicate elements, and its length does not exceed 248.
    1. Tradeoff: This places burden on the user to provide a specifically formatted block size list. However, with this assumption was made to accomodate O(log(b)) search for pool headers without having to sort the block size array during initialization, which would incur an additional O(b * log(b)) computational cost.
    1. The smallest x for which (2^16 - 16\*x) / x - x > 0 is x = 248.125.
    1. The largest block size count at which a valid set of block sizes can exist is therefore {1, 2, 3, 4, 5, ..., 246, 247, 248} which, 8-byte aligned, becomes {8, 8, 8, 8, 8, ..., 248, 248, 248}).

## Time and Space Complexity Analysis

```
b = number of pools (up to 248)
N = total number of blocks (up to 8190)
```

`pool_init`

Time - O(b + N)

Space - O(b + N)

`pool_alloc`:

Time - Best Case (Cache Hit): O(1), Average Case (Binary Search): O(log(b)), Worst Case (All Pools Full): O(b)

Space - Best Case (Cache Hit): O(1), Average Case: O(b)

`pool_free`:

Time - O(1)

Space - O(1)

### Further Future Optimizations
1. Populate block headers lazily as memory becomes allocated rather than all at once during initialization.
    1. This would lower initialization complexity to O(b) in both time and space. (See branch `lazy`)
1. External fragmentation between pools may be used to hold smaller size objects (e.g. leftover "dead" space after a 1024-byte pool before a subsequent 2048-byte pool may be split up into several 32-byte blocks).
    1. Filling leftover external fragmentation exclusively with the smallest memory block size leads to the greatest minimization of external fragmentation while also keeping in line with the assumption that small block size allocations are more desirable. (See branch `fragment`)
1. Compact all the pools together, combining all the leftover space at the end of the heap.
    1. This would get completely rid of external fragmentation, leaving one contiguous accumulated block of memory at the end of the heap to use however one wants. We initially chose not to do this to simplify pointer arithmetic for indexing into pools and their respective blocks. (See branch `compact`)

