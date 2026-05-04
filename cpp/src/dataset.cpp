#include "dataset.h"

#include <fstream>
#include <sstream>

namespace ccn2 {

Dataset load_dataset(const std::filesystem::path& file_path) {
    std::ifstream input(file_path);
    Dataset dataset;
    if (!input) {
        return dataset;
    }

    std::string line;
    while (std::getline(input, line)) {
        std::istringstream stream(line);
        Transaction transaction;
        int item = 0;
        while (stream >> item) {
            transaction.push_back(item);
        }

        transaction = normalize_transaction(std::move(transaction));
        if (!transaction.empty()) {
            dataset.push_back(std::move(transaction));
        }
    }

    return dataset;
}

}  // namespace ccn2
