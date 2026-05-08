#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ccbench {

using Transaction = std::vector<int>;
using Dataset = std::vector<Transaction>;
using Itemset = std::vector<int>;
using CountMap = std::unordered_map<std::string, int>;

struct MiningResult {
    CountMap frequent_itemsets;
    std::size_t candidate_count = 0;
};

struct RunResult {
    std::string dataset;
    std::string algorithm;
    double min_sup = 0.0;
    double seconds = 0.0;
    double peak_rss_mb = 0.0;
    std::size_t frequent_itemsets = 0;
    std::optional<std::size_t> candidate_count;
};

inline std::string encode_itemset(const Itemset& itemset) {
    std::string encoded;
    for (std::size_t i = 0; i < itemset.size(); ++i) {
        if (i > 0) {
            encoded.push_back(',');
        }
        encoded += std::to_string(itemset[i]);
    }
    return encoded;
}

inline Transaction normalize_transaction(Transaction transaction) {
    std::sort(transaction.begin(), transaction.end());
    transaction.erase(std::unique(transaction.begin(), transaction.end()), transaction.end());
    return transaction;
}

double current_rss_mb();

inline void update_peak(double* peak_rss_mb) {
    if (!peak_rss_mb) {
        return;
    }
    *peak_rss_mb = std::max(*peak_rss_mb, current_rss_mb());
}

}  // namespace ccbench
