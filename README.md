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

build\ccn2_cpp.exe --quick
Quick smoke test:

```powershell
cd D:\Code\DAAProject\cpp
./build/Release/ccn2_cpp.exe --quick
```

build\ccn2_cpp.exe --workers 8
Full sweep using 8 worker threads for the Apriori counting loops:

```powershell
cd D:\Code\DAAProject\cpp
./build/Release/ccn2_cpp.exe --workers 8
```

build\ccn2_cpp.exe --algorithms linear_table_sota
Run only one algorithm:

```powershell
cd D:\Code\DAAProject\cpp
./build/Release/ccn2_cpp.exe --algorithms linear_table_sota
```

## Notes

- The benchmark runner loads each dataset once and reuses it across repeats.
- `--workers` controls the parallel counting threads inside the Apriori algorithms.
- Peak RAM is sampled from the current process working set and is an approximate peak.


--- Average Results and Speedup Ratios ---
algorithm          linear_table_sota  optimized_apriori  original_apriori  speedup_optimized  speedup_sota
dataset   min_sup                                                                                         
accidents 0.85              0.513214           0.172803          0.203033           1.174943      0.395611
          0.90              0.298789           0.157580          0.106837           0.677984      0.357566
          0.95              0.183566           0.115095          0.063068           0.547966      0.343571
chess     0.85              0.168328           0.023675          0.026597           1.123437      0.158007
          0.90              0.046448           0.007100          0.007295           1.027370      0.157051
          0.95              0.007851           0.002680          0.003145           1.173632      0.400645
connect   0.85            736.539924         178.275720        138.372501           0.776171      0.187868
          0.90             89.556799          13.794228         16.223569           1.176113      0.181154
          0.95              9.750230           0.563784          0.723750           1.283737      0.074229

Scalability curves saved as PNG files.