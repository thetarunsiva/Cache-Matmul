# Interview Notes

## Core ideas

- **Row-major layout:** `Matrix::data[row * n + column]` places adjacent columns
  next to each other. A `std::vector<float>` owns one contiguous allocation, so
  sequential column indices have good spatial locality.
- **Naive `ijk`:** its inner loop keeps C[i,j] fixed but reads B[k,j] with an
  `n * sizeof(float)` stride. At N=1024 that is 4096 bytes, usually wasting most
  of every fetched 64-byte cache line.
- **Reordered `ikj`:** it loads A[i,k] once, then traverses B[k,j] and C[i,j]
  contiguously. This enables cache-line use, hardware prefetching, and easier
  compiler vectorization.
- **Tiling:** the six loops restrict work to submatrices so recently loaded A, B,
  and C data can be reused before eviction. Boundary minima handle dimensions
  that are not multiples of the tile size.
- **Tile size:** 16/32/64 imply simplified three-tile footprints of 3/12/48 KiB.
  The model `3 * tile² * 4` helps choose candidates but ignores associativity,
  replacement, cache lines, and how much of each tile is live simultaneously.
- **Cache hierarchy:** this CPU reports 32 KiB L1D and 512 KiB unified L2 per
  core, plus 16 MiB shared L3. Smaller levels are faster; misses fall through to
  the next level and ultimately DRAM.
- **Locality:** spatial locality uses nearby bytes already in a cache line;
  temporal locality reuses the same data before eviction. `ikj` mainly fixes
  spatial locality, while tiling aims to improve both.
- **`-O2` and vectorization:** `-O2` enables inlining, loop transformations, and
  auto-vectorization when legal. The compiler may use AVX instructions without
  source intrinsics, but a vectorization report or disassembly is needed to prove
  what happened.
- **C++ choices:** const references avoid copying input vectors; inline header
  functions avoid multiple-definition errors and expose code to optimization.
  Returning `Matrix` is efficient through copy elision/move semantics.
- **Floating point:** addition is not associative, so legal loop/order changes
  can change rounding. Tests use `|a-b| <= abs_tol + rel_tol * max(|a|,|b|)`
  and do not use `assert`, which disappears under `NDEBUG`.
- **CMake:** targets declare C++17, warnings, include paths, and `-O2`; CTest
  registers the standalone test executable. Release builds define `NDEBUG`.
- **perf:** cycles and instructions describe execution cost/work; generic
  cache misses are not specifically L1. `L1-dcache-load-misses` is the relevant
  event here, though WSL virtualizes PMU access and requires cautious interpretation.

## 15 interview questions

1. **Why is `ijk` slow for row-major B?**  The inner loop moves down a column,
   so consecutive loads are N floats apart and make poor use of cache lines.

2. **Why does `ikj` help?**  Its inner loop streams rows of B and C, turning
   strided accesses into contiguous accesses and reusing A[i,k].

3. **What does a cache line change?**  This CPU uses 64-byte lines, holding 16
   floats. Contiguous traversal can use all 16; a large stride may use only one.

4. **What is spatial versus temporal locality?**  Spatial locality accesses
   nearby addresses; temporal locality reuses the same address soon.

5. **How does tiling improve reuse?**  It completes work on bounded A/B/C regions
   before advancing, increasing the chance that reused data remains cached.

6. **Why was tile 64 fastest although its simple footprint exceeds L1D?**  The
   three-full-tile model overestimates simultaneously live data and ignores loop
   behavior; lower loop overhead and L2 reuse can outweigh an L1-only fit.

7. **Did tiling beat `ikj`?**  At N=256 and 512, tile 64 did. At N=1024, `ikj`
   was 1,782.846 ms versus 1,813.421 ms for tile 64, so `ikj` won by 1.7%.

8. **Why use a median?**  It resists occasional delays from scheduling and host
   activity better than a mean, especially under WSL2.

9. **What exactly was timed?**  One kernel call, including result allocation and
   zero-fill. Input creation, warm-up, correctness, checksum, and I/O were outside.

10. **How was dead-code elimination prevented?**  Every timed result was reduced
    to a checksum assigned to a volatile sink outside the timed interval.

11. **Why use both absolute and relative tolerance?**  Absolute tolerance works
    near zero; relative tolerance scales acceptance for large-magnitude values.

12. **Does `std::vector` guarantee contiguous storage?**  Yes. Its elements are
    contiguous, enabling row-major indexing and cache-friendly sequential loads.

13. **What did perf show at N=1024?**  `ikj` counted 69.39M L1D load misses versus
    1.237B for `ijk`, a 94.4% reduction, with almost identical instruction counts.

14. **Can generic `cache-misses` be called L1 misses?**  No. It is a PMU-defined
    last-level/general cache event; the explicitly named L1D event supports the claim.

15. **What would you optimize next?**  Pack or transpose B, inspect GCC
    vectorization reports, compare BLAS, then add SIMD and thread-level parallelism.

## 90-second project explanation

I built a single-threaded C++17 matrix multiplication project to quantify memory
locality on my AMD Ryzen 5 5600H. All matrices use a flat row-major
`std::vector<float>`. I kept three readable kernels: conventional `ijk`, reordered
`ikj`, and a cache-blocked version with measured tile sizes 16, 32, and 64.

The key issue is that `ijk` reads B down columns. At 1024, consecutive inner-loop
loads are 4096 bytes apart, so it wastes cache-line spatial locality. Reordering
to `ikj` walks rows of B and C contiguously. I validated the algorithms with 846
checks, including known answers, partial tiles, invalid inputs, and floating-point
tolerance, then used a steady-clock harness with deterministic inputs, one warm-up,
seven repetitions, median timing, and an observable checksum.

At 1024×1024, `ijk` took 14,972.553 milliseconds and `ikj` took 1,782.846
milliseconds, an 8.40-times speedup. Tile 64 was the best blocked choice at
1,813.421 milliseconds, so I report honestly that plain `ikj` was slightly faster
at the largest size. Repeated perf measurements counted 1.237 billion L1D load
misses for `ijk` and 69.39 million for `ikj`, a 94.4% reduction while instruction
counts stayed nearly equal. I also retained raw CSV and perf output, generated a
dependency-free plot, integrated CTest, and added a small GitHub Actions workflow.
Because this ran under WSL2, I separate steady-clock timing claims from virtualized
PMU results and document counter variability rather than hiding it.
