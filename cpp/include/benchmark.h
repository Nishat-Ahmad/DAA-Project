#pragma once

#include "common.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ccn2 {

struct BenchmarkOptions {
    unsigned workers = 0;
    bool quick = false;
    std::vector<std::string> algorithms;
    std::vector<std::string> datasets;
    std::filesystem::path data_root;
    std::filesystem::path output_csv = "results.csv";
    std::size_t repeats = 3;
};

BenchmarkOptions parse_arguments(int argc, char** argv);
int run_benchmark(const BenchmarkOptions& options);

}  // namespace ccn2
