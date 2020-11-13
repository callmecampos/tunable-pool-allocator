# Zipline Take-Home Challenge: Tunable Block Pool Allocator

### How to compile and run: (on Unix-based OS, see Makefiles for Windows-specific instructions)

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

### High-level implementation:

![High-level implementation diagram from Notability.](./tunable-block-pool-allocator-diagram.png "High-level implementation diagram from Notability.")

### Assumptions:

Conceptual assumptions:
1. For this particular memory allocator, the smaller block size allocation takes precedent over larger block sizes, which informs many of the design decisions made in building the allocator. (e.g. filling in larger memory blocks, fragment space, etc. with smaller or smallest memory blocks --> this also leads to greatest minimization of fragmentation when possible, with the tradeoff being embedded in the assumption itself, that larger memory allocation has lower precedence/priority).

Logistical assumptions:
1. Heap itself can be used to store state.
1. Heap is subdivided evenly by number of pools, giving smaller objects more blocks to allocate into.
1. The block sizes array is passed in pre-sorted smallest to largest.
    1. This assumption was made to accomodate O(log(N)) search for pool headers.
1. No duplicate block sizes are passed into the pool initialization function.
1. No individual block size exceeds its respective allocated pool size.
1. The number of block sizes doesn't exceed 248.
    1. Smallest x for which (2^16 - 16\*x) / x - x > 0 is at x = 248.125.
    1. The largest block size count at which a valid set of block sizes can exist is therefore {1, 2, 3, 4, 5, ..., 246, 247, 248} which, 8-byte aligned, becomes {8, 8, 8, 8, 8, ..., 248, 248, 248}).
1. pool_free() has undefined behavior when passed a pointer is not currently allocated by pool_alloc() (whether because it wasn't allocated in the first place or it was already freed).
1. pool_init() is only called once per a process.

### Tradeoffs:

1. Storing state in the provided heap: This method ensures simplicity and elegance in the implementation. Tradeoff is that you lose some memory, but not much at all really.
1. 8-byte alignment (for 64-bit processor): Want to make sure memory accesses are as efficient as possible, so are willing to tradeoff fragmentation for speed.

### Time and Space Complexity Analysis

```
b = number of pools
N = total number of blocks
```

`pool_init`
Time: O(b + N)
Space: O(b + N)

`pool_alloc`:
Time: O(log(b))
Space: O(b)

`pool_free`:
Time: O(1)
Space: O(1)

#### Further Optimizations
1. Don't populate all block headers at once during initialization. Do it lazily as memory becomes allocated.
2. External fragmentation between pools may be used to hold smaller size objects (e.g. leftover space after a 1024-byte pool may be split up into several 32-byte blocks). We could have alternatively compacted all the pools together, combining all the leftover space at the end of the heap, but chose not to do this to simplify pointer arithmetic for indexing into pools and their respective blocks.

