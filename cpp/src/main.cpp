#include "benchmark.h"

int main(int argc, char** argv) {
    const auto options = ccn2::parse_arguments(argc, argv);
    return ccn2::run_benchmark(options);
}
