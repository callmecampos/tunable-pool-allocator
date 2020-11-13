### Zipline Take-Home Challenge: Tunable Block Pool Allocator

High-level implementation:

[Implementation diagram from Notability]

Assumptions:

1. Heap itself can be used to store state.
2. Even subdivision of heap (divide evenly by number of pools e.g. 4 block sizes each get the same amount of heap space, giving smaller objects more blocks to allocate into).
    a. Can also enable reasonable subdivision of heap (e.g. larger objects get larger pools, smaller objects get smaller pools).
3. The block sizes array is passed in pre-sorted smallest to largest.
    a. This assumption was made to accomodate O(log(N)) search for pool headers.
4. No duplicate block sizes are passed into the pool initialization function.
5. No individual block size exceeds its respective allocated pool size.
    a. Following from this, it is assumed that no individual block size exceeds 2^16 - 16 = 65520 to allow for pool header space.
6. Number of block sizes doesn't exceed 248 ((2^16 - 16\*x) = x for x = 248.125. Largest block size count at which a valid set of block sizes can exist is therefore {1, 2, 3, 4, 5, ..., 246, 247, 248} which, 8-byte aligned, becomes {8, 8, 8, 8, 8, ..., 248, 248, 248}).

Tradeoffs:

1. Storing state in the provided heap: This method ensures simplicity and elegance in the implementation. Tradeoff is that you lose some memory, but not much at all really.
2. 8-byte alignment (for 64-bit processor): Want to make sure memory accesses are as efficient as possible, so are willing to tradeoff fragmentation for speed. This can be disabled by setting BYTE_ALIGNMENT in pool_alloc.h to false.
3. External fragmentation between pools may be used to hold smaller size objects (e.g. leftover space after a 1024-byte pool may be split up into several 32-byte blocks). We could have alternatively compacted all the pools together, combining all the leftover space at the end of the heap, but chose not to do this to simplify pointer arithmetic for indexing into pools and their respective blocks.


---

How to compile and run:

[Compilation and running instructions here]
