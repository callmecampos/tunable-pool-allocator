### Zipline Take-Home Challenge: Tunable Block Pool Allocator

Assumptions:

1. Heap can be used to store state.
2. Reasonable subdivision of heap (divide evenly by number of pools e.g. 4 block sizes each get the same amount of heap space, giving smaller objects more blocks to allocate into)
3. Number of block sizes doesn't exceed 248 ((2^16 - 16\*x) = x for x = 248.125. Largest block size count at which a valid set of block sizes can exist is therefore {1, 2, 3, 4, 5, ..., 248}).
4. No individual block size exceeds the evenly divided pool size (2^16 / block\_size\_count e.g. for 2^16 / 2^7 = 512, no block size can exceed 512 bytes).

High-level implementation:

[Implementation diagram from Notability]

---

How to compile and run:

[Compilation and running instructions here]
