import csv
import gc
import os
import sys
import tracemalloc
from pathlib import Path
from typing import Callable, Dict, List

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from aprori import load_dataset, apriori_original, apriori_optimized_with_stats  # noqa: E402
from linearTable import LinearTableMiner  # noqa: E402


DATASETS: Dict[str, Path] = {
    "chess": PROJECT_ROOT / "Dataset" / "chess.dat",
    "connect": PROJECT_ROOT / "Dataset" / "connect.dat",
    "accidents": PROJECT_ROOT / "Dataset" / "accidents.dat",
}

MIN_SUPS: Dict[str, List[float]] = {
    "chess": [0.9, 0.8, 0.7],
    "connect": [0.9, 0.8, 0.7],
    "accidents": [0.98, 0.95],
}

REPEATS = 3
OUTPUT_CSV = PROJECT_ROOT / "results.csv"


def count_itemsets(frequent_itemsets) -> int:
    return len(frequent_itemsets)


def get_process_memory_mb() -> float | None:
    try:
        import psutil  # type: ignore

        process = psutil.Process(os.getpid())
        return process.memory_info().rss / (1024 * 1024)
    except Exception:
        return None


def measure_peak_memory(func: Callable, *args, **kwargs):
    gc.collect()
    tracemalloc.start()
    process_start = get_process_memory_mb()
    result = func(*args, **kwargs)
    _, peak_alloc = tracemalloc.get_traced_memory()
    tracemalloc.stop()
    process_end = get_process_memory_mb()

    peak_mb = peak_alloc / (1024 * 1024)
    if process_start is not None and process_end is not None:
        peak_mb = max(peak_mb, process_end - process_start)
    return result, peak_mb


def linear_table_run(dataset: List[set], min_sup: float):
    miner = LinearTableMiner(min_sup)
    frequent_itemsets, execution_time = miner.mine(dataset)
    return frequent_itemsets, execution_time


def run_with_metrics(method_name: str, runner: Callable, dataset: List[set], min_sup: float):
    gc.collect()
    (result, peak_mb) = measure_peak_memory(runner, dataset, min_sup)
    if method_name in {"linear_table", "linear_table_sota"}:
        frequent_itemsets, execution_time = result
        candidate_count = "NA"
    else:
        frequent_itemsets, execution_time, candidate_count = result

    return {
        "algorithm": method_name,
        "average_time": execution_time,
        "peak_ram_mb": peak_mb,
        "frequent_itemsets": count_itemsets(frequent_itemsets),
        "candidate_count": candidate_count,
    }


def summarize_runs(rows: List[Dict]) -> Dict:
    summary = dict(rows[0])
    numeric_keys = ["average_time", "peak_ram_mb", "frequent_itemsets"]
    for key in numeric_keys:
        summary[key] = sum(row[key] for row in rows) / len(rows)

    candidate_values = [row["candidate_count"] for row in rows if isinstance(row["candidate_count"], (int, float))]
    if candidate_values:
        summary["candidate_count"] = sum(candidate_values) / len(candidate_values)
    else:
        summary["candidate_count"] = "NA"
    return summary


def main():
    results = []

    for dataset_name, dataset_path in DATASETS.items():
        print(f"Loading {dataset_name} from {dataset_path.name}...", flush=True)
        dataset = load_dataset(str(dataset_path))
        print(f"Loaded {dataset_name} with {len(dataset)} transactions.", flush=True)
        for min_sup in MIN_SUPS[dataset_name]:
            print(f"Running benchmarks for {dataset_name} at min_sup={min_sup}...", flush=True)
            per_run = {
                "original_apriori": [],
                "optimized_apriori": [],
                "linear_table_sota": [],
            }

            for repeat_idx in range(REPEATS):
                print(f"  Repeat {repeat_idx + 1}/{REPEATS}: original_apriori", flush=True)
                per_run["original_apriori"].append(
                    run_with_metrics("original_apriori", apriori_original, dataset, min_sup)
                )
                print(f"  Repeat {repeat_idx + 1}/{REPEATS}: optimized_apriori", flush=True)
                per_run["optimized_apriori"].append(
                    run_with_metrics("optimized_apriori", apriori_optimized_with_stats, dataset, min_sup)
                )
                print(f"  Repeat {repeat_idx + 1}/{REPEATS}: linear_table_sota", flush=True)
                per_run["linear_table_sota"].append(
                    run_with_metrics("linear_table_sota", linear_table_run, dataset, min_sup)
                )

            for algorithm_name, rows in per_run.items():
                summary = summarize_runs(rows)
                summary["dataset"] = dataset_name
                summary["min_sup"] = min_sup
                summary["algorithm"] = algorithm_name
                results.append(summary)
                print(
                    f"  Saved average for {algorithm_name}: "
                    f"time={summary['average_time']:.4f}s, "
                    f"RAM={summary['peak_ram_mb']:.2f}MB, "
                    f"itemsets={summary['frequent_itemsets']}",
                    flush=True,
                )

    with open(OUTPUT_CSV, "w", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(
            csv_file,
            fieldnames=[
                "dataset",
                "algorithm",
                "min_sup",
                "average_time",
                "peak_ram_mb",
                "frequent_itemsets",
                "candidate_count",
            ],
        )
        writer.writeheader()
        writer.writerows(results)

    print(f"Saved {len(results)} averaged rows to {OUTPUT_CSV}")


if __name__ == "__main__":
    main()