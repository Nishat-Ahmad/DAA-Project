#include "benchmark.h"

int main(int argc, char** argv) {
    const auto options = ccbench::parse_arguments(argc, argv);
    return ccbench::run_benchmark(options);
}
