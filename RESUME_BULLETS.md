# Resume bullets

- Built a single-threaded C++17 matrix-multiplication benchmark using flat row-major `std::vector<float>` storage, implementing readable `ijk`, `ikj`, and cache-blocked kernels and validating matrices through 1024×1024.

- Accelerated 1024×1024 multiplication 8.40× on an AMD Ryzen 5 5600H by reordering loops for contiguous B/C access, reducing median runtime from 14,972.553 ms to 1,782.846 ms across seven repetitions.

- Measured 94.4% fewer L1 data-cache load misses (1.237B to 69.39M) with perf, and delivered reproducible CMake/CTest builds, CSV benchmarks, plots, and GitHub Actions CI configuration.
