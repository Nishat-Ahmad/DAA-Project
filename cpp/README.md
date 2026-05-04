# CCN2 C++ Rewrite

This directory contains a C++ rewrite of the CCN2 frequent-itemset mining project.

## What is included

- dataset loader for the `Dataset/*.dat` files
- original Apriori
- optimized Apriori with transaction tidying
- linear-table miner
- benchmark CLI that can run quick tests or full sweeps
- CSV output compatible with the Python workflow

## Build

```bash
cd cpp
cmake -S . -B build
cmake --build build --config Release
```

## Run

Quick smoke test:

```bash
.
build\ccn2_cpp.exe --quick
```

Full sweep using 8 worker threads for the Apriori counting loops:

```bash
.
build\ccn2_cpp.exe --workers 8
```

Run only one algorithm:

```bash
.
build\ccn2_cpp.exe --algorithms linear_table_sota
```

## Notes

- The benchmark runner loads each dataset once and reuses it across repeats.
- `--workers` controls the parallel counting threads inside the Apriori algorithms.
- Peak RAM is sampled from the current process working set and is an approximate peak.
