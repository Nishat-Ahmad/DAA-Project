#include "benchmark.h"

#include "apriori.h"
#include "dataset.h"
#include "linear_table.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace ccbench {

double current_rss_mb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters))) {
        return static_cast<double>(counters.WorkingSetSize) / (1024.0 * 1024.0);
    }
    return 0.0;
#else
    return 0.0;
#endif
}

namespace {

std::string trim(std::string value) {
    const auto begin = value.find_first_not_of(" \t\n\r");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\n\r");
    return value.substr(begin, end - begin + 1);
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = trim(std::move(token));
        if (!token.empty()) {
            parts.push_back(std::move(token));
        }
    }
    return parts;
}

std::filesystem::path discover_dataset_root(std::filesystem::path requested_root) {
    const std::vector<std::filesystem::path> candidates = {
        requested_root,
        std::filesystem::current_path() / "Dataset",
        std::filesystem::current_path().parent_path() / "Dataset",
        std::filesystem::current_path().parent_path().parent_path() / "Dataset",
    };

    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate / "chess.dat")) {
            return candidate;
        }
    }

    return requested_root.empty() ? std::filesystem::current_path() / "Dataset" : requested_root;
}

std::vector<std::string> default_algorithms() {
    return {"original_apriori", "optimized_apriori", "linear_table_sota"};
}

std::vector<std::string> default_datasets(bool quick) {
    if (quick) {
        return {"chess"};
    }
    return {"chess", "connect", "accidents"};
}

std::vector<double> min_sups_for_dataset(const std::string& dataset_name, bool quick) {
    if (quick) {
        return {0.9};
    }
    return {0.95, 0.9, 0.85};
}

MiningResult run_algorithm(const std::string& algorithm, const Dataset& dataset, double min_sup, unsigned workers,
                           double* peak_rss_mb) {
    if (algorithm == "original_apriori") {
        return apriori_original(dataset, min_sup, workers, peak_rss_mb);
    }
    if (algorithm == "optimized_apriori") {
        return apriori_optimized(dataset, min_sup, workers, peak_rss_mb);
    }
    if (algorithm == "linear_table_sota") {
        return linear_table_mine(dataset, min_sup, peak_rss_mb);
    }
    throw std::runtime_error("Unknown algorithm: " + algorithm);
}

void write_csv_header(std::ofstream& output) {
    output << "dataset,algorithm,min_sup,average_time,peak_ram_mb,frequent_itemsets,candidate_count\n";
}

void write_csv_row(std::ofstream& output, const RunResult& row) {
    output << row.dataset << ',' << row.algorithm << ',' << std::fixed << std::setprecision(2) << row.min_sup << ','
           << std::setprecision(6) << row.seconds << ',' << std::setprecision(2) << row.peak_rss_mb << ','
           << row.frequent_itemsets << ',';
    if (row.candidate_count.has_value()) {
        output << *row.candidate_count;
    } else {
        output << "NA";
    }
    output << '\n';
}

}  // namespace

BenchmarkOptions parse_arguments(int argc, char** argv) {
    BenchmarkOptions options;
    options.algorithms = default_algorithms();
    options.datasets = default_datasets(false);
    options.data_root = "Dataset";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--quick") {
            options.quick = true;
            options.datasets = default_datasets(true);
            continue;
        }
        if (arg == "--workers" || arg == "-w") {
            if (i + 1 < argc) {
                options.workers = static_cast<unsigned>(std::stoul(argv[++i]));
            }
            continue;
        }
        if (arg == "--algorithms" || arg == "-a") {
            if (i + 1 < argc) {
                options.algorithms = split_csv(argv[++i]);
            }
            continue;
        }
        if (arg == "--datasets" || arg == "-d") {
            if (i + 1 < argc) {
                options.datasets = split_csv(argv[++i]);
            }
            continue;
        }
        if (arg == "--repeats" || arg == "-r") {
            if (i + 1 < argc) {
                options.repeats = static_cast<std::size_t>(std::stoul(argv[++i]));
            }
            continue;
        }
        if (arg == "--data-root") {
            if (i + 1 < argc) {
                options.data_root = argv[++i];
            }
            continue;
        }
        if (arg == "--output" || arg == "-o") {
            if (i + 1 < argc) {
                options.output_csv = argv[++i];
            }
            continue;
        }
    }

    if (options.workers == 0) {
        const auto hw = std::thread::hardware_concurrency();
        options.workers = hw == 0 ? 1 : hw;
    }

    if (options.quick) {
        options.repeats = 1;
    }

    return options;
}

int run_benchmark(const BenchmarkOptions& options) {
    const auto dataset_root = discover_dataset_root(options.data_root);
    std::unordered_map<std::string, Dataset> datasets;
    std::unordered_map<std::string, std::size_t> dataset_sizes;

    for (const auto& dataset_name : options.datasets) {
        const auto file_path = dataset_root / (dataset_name + ".dat");
        std::cout << "Loading " << dataset_name << " from " << file_path.string() << "..." << std::endl;
        datasets.emplace(dataset_name, load_dataset(file_path));
        dataset_sizes[dataset_name] = datasets.at(dataset_name).size();
        std::cout << "Loaded " << dataset_name << " with " << dataset_sizes[dataset_name] << " transactions."
                  << std::endl;
    }

    std::vector<RunResult> rows;
    rows.reserve(options.datasets.size() * options.repeats * options.algorithms.size() * 3);

    for (const auto& dataset_name : options.datasets) {
        const auto& dataset = datasets.at(dataset_name);
        const auto min_sups = min_sups_for_dataset(dataset_name, options.quick);

        for (double min_sup : min_sups) {
            std::cout << "Running benchmarks for " << dataset_name << " at min_sup=" << min_sup << "..." << std::endl;
            for (std::size_t repeat = 1; repeat <= options.repeats; ++repeat) {
                for (const auto& algorithm : options.algorithms) {
                    std::cout << "  Repeat " << repeat << "/" << options.repeats << ": " << algorithm << std::endl;

                    double peak_rss_mb = current_rss_mb();
                    const auto started = std::chrono::steady_clock::now();
                    const MiningResult result = run_algorithm(algorithm, dataset, min_sup, options.workers, &peak_rss_mb);
                    const auto finished = std::chrono::steady_clock::now();

                    update_peak(&peak_rss_mb);
                    const std::chrono::duration<double> elapsed = finished - started;

                    RunResult row;
                    row.dataset = dataset_name;
                    row.algorithm = algorithm;
                    row.min_sup = min_sup;
                    row.seconds = elapsed.count();
                    row.peak_rss_mb = peak_rss_mb;
                    row.frequent_itemsets = result.frequent_itemsets.size();
                    if (algorithm == "linear_table_sota") {
                        row.candidate_count = std::nullopt;
                    } else {
                        row.candidate_count = result.candidate_count;
                    }

                    rows.push_back(row);
                    std::cout << "    time=" << std::fixed << std::setprecision(2) << row.seconds << "s"
                              << " ram=" << std::setprecision(2) << row.peak_rss_mb << "MB"
                              << " items=" << row.frequent_itemsets << std::endl;
                }
            }
        }
    }

    std::ofstream output(options.output_csv);
    if (!output) {
        std::cerr << "Failed to open output CSV: " << options.output_csv.string() << std::endl;
        return 1;
    }

    write_csv_header(output);
    for (const auto& row : rows) {
        write_csv_row(output, row);
    }

    std::cout << "Saved " << rows.size() << " raw rows to " << options.output_csv.string() << std::endl;
    return 0;
}

}  // namespace ccbench
