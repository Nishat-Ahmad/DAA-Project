import csv
import gc
import os
import sys
import time
import tracemalloc
from pathlib import Path
from typing import Callable, Dict, Iterable, List, Tuple

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from aprori import load_dataset, get_frequent_1_itemsets, generate_candidates, apriori_optimized  # noqa: E402
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


def apriori_original(dataset: List[set], min_sup: float):
    start_time = time.time()
    n_transactions = len(dataset)
    frequent_itemsets = {}
    candidate_count = 0

    current_frequent = get_frequent_1_itemsets(dataset, min_sup)
    frequent_itemsets.update(current_frequent)

    k = 2
    prev_frequent_keys = list(current_frequent.keys())
    while prev_frequent_keys:
        candidates = set()
        for i in range(len(prev_frequent_keys)):
            for j in range(i + 1, len(prev_frequent_keys)):
                left = sorted(prev_frequent_keys[i])
                right = sorted(prev_frequent_keys[j])
                if left[: k - 2] == right[: k - 2]:
                    candidates.add(prev_frequent_keys[i] | prev_frequent_keys[j])

        if not candidates:
            break

        candidate_count += len(candidates)
        candidate_support = {candidate: 0 for candidate in candidates}
        for transaction in dataset:
            for candidate in candidates:
                if candidate.issubset(transaction):
                    candidate_support[candidate] += 1

        current_frequent = {
            candidate: support
            for candidate, support in candidate_support.items()
            if support / n_transactions >= min_sup
        }

        if not current_frequent:
            break

        frequent_itemsets.update(current_frequent)
        prev_frequent_keys = list(current_frequent.keys())
        k += 1

    execution_time = time.time() - start_time
    return frequent_itemsets, execution_time, candidate_count


def apriori_optimized_with_candidates(dataset: List[set], min_sup: float):
    start_time = time.time()
    n_initial = len(dataset)

    current_frequent_dict = get_frequent_1_itemsets(dataset, min_sup)
    all_frequent = dict(current_frequent_dict)
    frequent_items_flat = set().union(*current_frequent_dict.keys()) if current_frequent_dict else set()

    candidate_count = 0
    k = 2
    while current_frequent_dict:
        candidates = generate_candidates(current_frequent_dict.keys(), k)
        if not candidates:
            break

        candidate_count += len(candidates)
        candidate_counts = {can: 0 for can in candidates}

        new_dataset = []
        for transaction in dataset:
            filtered_tx = transaction.intersection(frequent_items_flat)
            if len(filtered_tx) >= k:
                new_dataset.append(filtered_tx)
                for can in candidates:
                    if can.issubset(filtered_tx):
                        candidate_counts[can] += 1

        dataset = new_dataset
        current_frequent_dict = {
            can: count for can, count in candidate_counts.items() if (count / n_initial) >= min_sup
        }

        if current_frequent_dict:
            all_frequent.update(current_frequent_dict)
            frequent_items_flat = set().union(*current_frequent_dict.keys())

        k += 1

    execution_time = time.time() - start_time
    return all_frequent, execution_time, candidate_count


def linear_table_run(dataset: List[set], min_sup: float):
    miner = LinearTableMiner(min_sup)
    frequent_itemsets, execution_time = miner.mine(dataset)
    return frequent_itemsets, execution_time


def run_with_metrics(method_name: str, runner: Callable, dataset: List[set], min_sup: float):
    gc.collect()
    (result, peak_mb) = measure_peak_memory(runner, dataset, min_sup)
    if method_name == "linear_table":
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
        dataset = load_dataset(str(dataset_path))
        for min_sup in MIN_SUPS[dataset_name]:
            per_run = {
                "original_apriori": [],
                "optimized_apriori": [],
                "linear_table_sota": [],
            }

            for _ in range(REPEATS):
                per_run["original_apriori"].append(
                    run_with_metrics("original_apriori", apriori_original, dataset, min_sup)
                )
                per_run["optimized_apriori"].append(
                    run_with_metrics("optimized_apriori", apriori_optimized_with_candidates, dataset, min_sup)
                )
                per_run["linear_table_sota"].append(
                    run_with_metrics("linear_table_sota", linear_table_run, dataset, min_sup)
                )

            for algorithm_name, rows in per_run.items():
                summary = summarize_runs(rows)
                summary["dataset"] = dataset_name
                summary["min_sup"] = min_sup
                summary["algorithm"] = algorithm_name
                results.append(summary)

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