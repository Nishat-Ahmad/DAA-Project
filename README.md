# Performance Evaluation of Frequent Itemset Mining: Classical Apriori vs. Linear Tables

This repository contains a frequent-itemset mining benchmark for the Design and Analysis of Algorithms project.

This project implements and benchmarks multiple frequent-itemset mining algorithms (original Apriori, optimized Apriori, and a linear-table miner) on several datasets, measuring runtime, peak memory, candidate counts, and frequent-itemset outputs; results are saved to `cpp/results.csv` and visualized in `Graphs/`.

## What it does

- Loads the transaction datasets in `Dataset/`
- Runs three mining algorithms:
    - original Apriori
    - optimized Apriori
    - linear-table miner
- Measures runtime, memory usage, and candidate counts
- Writes benchmark results to `results.csv`

## Project layout

- `cpp/` - C++ implementation, build files, and benchmark CLI (`ccbench_cpp`)
- `Dataset/` - benchmark datasets
- `md/` - reports and submission notes
- `results.csv` - latest benchmark output

## Build

```powershell
cd D:\Code\DAAProject\cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Run

Quick test:

```powershell
cd D:\Code\DAAProject\cpp
./build/Release/ccbench_cpp.exe --quick
```

Full benchmark:

```powershell
cd D:\Code\DAAProject\cpp
./build/Release/ccbench_cpp.exe --workers 8
```

## Notes

- The benchmark uses the local `Dataset/` folder by default.
- `--workers` controls the parallel support-counting threads used by the Apriori algorithms.
- The current benchmark thresholds are tuned for faster runs while still showing scaling behavior.