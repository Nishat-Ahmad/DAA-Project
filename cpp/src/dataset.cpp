#include "dataset.h"

#include <fstream>
#include <iostream>
#include <cstdlib>

namespace ccn2 {

Dataset load_dataset(const std::filesystem::path& file_path) {
    Dataset dataset;
    std::ifstream input(file_path);
    if (!input) {
        std::cerr << "Failed to open dataset file: " << file_path << "\n";
        return dataset;
    }

    std::string line;
    std::size_t line_no = 0;
    while (std::getline(input, line)) {
        ++line_no;
        if (line.empty()) {
            continue;
        }

        Transaction transaction;
        transaction.reserve(16);

        const char* p = line.c_str();
        char* endptr = nullptr;
        while (*p != '\0') {
            // strtol handles spaces and stops at non-numeric
            long v = std::strtol(p, &endptr, 10);
            if (endptr == p) {
                // skip invalid char
                ++p;
                continue;
            }
            transaction.push_back(static_cast<int>(v));
            p = endptr;
        }

        transaction = normalize_transaction(std::move(transaction));
        if (!transaction.empty()) {
            dataset.push_back(std::move(transaction));
        }
    }

    if (dataset.empty()) {
        std::cerr << "Warning: dataset " << file_path << " loaded but contains no transactions.\n";
    }

    return dataset;
}

}  // namespace ccn2
