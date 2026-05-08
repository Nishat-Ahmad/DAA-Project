#include "linear_table.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace ccn2 {

namespace {

void apriori_recursive_linear_table(const std::vector<Transaction>& current_table, const Itemset& prefix,
                                    const std::vector<int>& items_list, std::size_t minimum_support,
                                    MiningResult& result, double* peak_rss_mb) {
    for (int index = static_cast<int>(items_list.size()) - 1; index >= 0; --index) {
        Itemset next_prefix = prefix;
        next_prefix.push_back(items_list[static_cast<std::size_t>(index)]);

        std::size_t support = 0;
        std::vector<Transaction> new_table;
        for (const auto& transaction : current_table) {
            const auto it = std::lower_bound(transaction.begin(), transaction.end(), index);
            if (it != transaction.end() && *it == index) {
                ++support;
                const auto offset = static_cast<std::size_t>(std::distance(transaction.begin(), it));
                if (offset > 0) {
                    new_table.emplace_back(transaction.begin(), transaction.begin() + static_cast<std::ptrdiff_t>(offset));
                }
            }
        }

        if (support >= minimum_support) {
            result.frequent_itemsets[encode_itemset(next_prefix)] = static_cast<int>(support);
            update_peak(peak_rss_mb);

            if (!new_table.empty() && index > 0) {
                std::vector<int> next_items(items_list.begin(), items_list.begin() + index);
                apriori_recursive_linear_table(new_table, next_prefix, next_items, minimum_support, result, peak_rss_mb);
            }
        }
    }
}

}  // namespace

MiningResult linear_table_mine(const Dataset& dataset, double min_sup, double* peak_rss_mb) {
    MiningResult result;
    if (dataset.empty()) {
        return result;
    }

    std::unordered_map<int, std::size_t> item_counts;
    for (const auto& transaction : dataset) {
        for (int item : transaction) {
            ++item_counts[item];
        }
    }

    const std::size_t minimum_support = static_cast<std::size_t>(std::ceil(min_sup * static_cast<double>(dataset.size())));

    std::vector<std::pair<int, std::size_t>> frequent_items;
    frequent_items.reserve(item_counts.size());
    for (const auto& [item, count] : item_counts) {
        if (count >= minimum_support) {
            frequent_items.emplace_back(item, count);
        }
    }

    std::sort(frequent_items.begin(), frequent_items.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.second != rhs.second) {
            return lhs.second > rhs.second;
        }
        return lhs.first < rhs.first;
    });

    std::unordered_map<int, int> rank_lookup;
    for (std::size_t index = 0; index < frequent_items.size(); ++index) {
        rank_lookup[frequent_items[index].first] = static_cast<int>(index);
    }

    std::vector<Transaction> linear_table;
    linear_table.reserve(dataset.size());
    for (const auto& transaction : dataset) {
        Transaction filtered;
        filtered.reserve(transaction.size());
        for (int item : transaction) {
            const auto it = rank_lookup.find(item);
            if (it != rank_lookup.end()) {
                filtered.push_back(it->second);
            }
        }
        std::sort(filtered.begin(), filtered.end());
        if (!filtered.empty()) {
            linear_table.push_back(std::move(filtered));
        }
    }

    update_peak(peak_rss_mb);

    Itemset prefix;
    std::vector<int> items_list;
    items_list.reserve(frequent_items.size());
    for (const auto& [item, _] : frequent_items) {
        items_list.push_back(item);
    }

    apriori_recursive_linear_table(linear_table, prefix, items_list, minimum_support, result, peak_rss_mb);
    return result;
}

}  // namespace ccn2