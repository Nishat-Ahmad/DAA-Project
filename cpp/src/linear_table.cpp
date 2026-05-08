#include "linear_table.h"

#include <algorithm>
#include <cmath>
#include <thread>
#include <unordered_map>

namespace ccbench {

namespace {

struct LinearTableFrame {
    std::vector<Transaction> table;
    Itemset prefix;
    std::vector<int> items_list;
    int next_index;
};

struct ProjectionResult {
    std::size_t support = 0;
    std::vector<Transaction> projected_table;
};

ProjectionResult project_table(const std::vector<Transaction>& table, int index, std::size_t minimum_support,
                               unsigned workers) {
    ProjectionResult result;
    if (table.empty()) {
        return result;
    }

    unsigned thread_count = workers == 0 ? std::thread::hardware_concurrency() : workers;
    if (thread_count == 0) {
        thread_count = 1;
    }
    thread_count = static_cast<unsigned>(std::min<std::size_t>(thread_count, table.size()));

    if (thread_count <= 1 || table.size() < 256) {
        for (const auto& transaction : table) {
            const auto it = std::lower_bound(transaction.begin(), transaction.end(), index);
            if (it != transaction.end() && *it == index) {
                ++result.support;
            }
        }
    } else {
        std::vector<std::size_t> local_supports(thread_count, 0);
        std::vector<std::thread> pool;
        pool.reserve(thread_count);

        const std::size_t chunk_size = (table.size() + thread_count - 1) / thread_count;
        for (unsigned thread_index = 0; thread_index < thread_count; ++thread_index) {
            const std::size_t begin = thread_index * chunk_size;
            const std::size_t end = std::min(table.size(), begin + chunk_size);
            if (begin >= end) {
                break;
            }

            pool.emplace_back([thread_index, begin, end, index, &table, &local_supports]() {
                auto& support = local_supports[thread_index];
                for (std::size_t row_index = begin; row_index < end; ++row_index) {
                    const auto& transaction = table[row_index];
                    const auto it = std::lower_bound(transaction.begin(), transaction.end(), index);
                    if (it != transaction.end() && *it == index) {
                        ++support;
                    }
                }
            });
        }

        for (auto& worker : pool) {
            worker.join();
        }

        for (const auto support : local_supports) {
            result.support += support;
        }
    }

    if (result.support < minimum_support) {
        return result;
    }

    result.projected_table.reserve(table.size());
    for (const auto& transaction : table) {
        const auto it = std::lower_bound(transaction.begin(), transaction.end(), index);
        if (it != transaction.end() && *it == index) {
            const auto offset = static_cast<std::size_t>(std::distance(transaction.begin(), it));
            if (offset > 0) {
                result.projected_table.emplace_back(transaction.begin(),
                                                   transaction.begin() + static_cast<std::ptrdiff_t>(offset));
            }
        }
    }

    return result;
}

void linear_table_dfs_iterative(std::vector<Transaction> initial_table, std::vector<int> initial_items,
                                std::size_t minimum_support, unsigned workers, MiningResult& result,
                                double* peak_rss_mb) {
    std::vector<LinearTableFrame> stack;
    const int initial_index = static_cast<int>(initial_items.size()) - 1;
    stack.push_back({std::move(initial_table), Itemset{}, std::move(initial_items), initial_index});

    while (!stack.empty()) {
        LinearTableFrame& frame = stack.back();
        if (frame.next_index < 0) {
            stack.pop_back();
            continue;
        }

        const int index = frame.next_index;
        --frame.next_index;

        Itemset next_prefix = frame.prefix;
        next_prefix.push_back(frame.items_list[static_cast<std::size_t>(index)]);

        const ProjectionResult projection = project_table(frame.table, index, minimum_support, workers);
        if (projection.support < minimum_support) {
            continue;
        }

        result.frequent_itemsets[encode_itemset(next_prefix)] = static_cast<int>(projection.support);
        update_peak(peak_rss_mb);

        if (!projection.projected_table.empty() && index > 0) {
            std::vector<int> next_items(frame.items_list.begin(), frame.items_list.begin() + index);
            stack.push_back({std::move(projection.projected_table), std::move(next_prefix), std::move(next_items),
                             index - 1});
        }
    }
}

}  // namespace

MiningResult linear_table_mine(const Dataset& dataset, double min_sup, unsigned workers, double* peak_rss_mb) {
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

    linear_table_dfs_iterative(std::move(linear_table), std::move(items_list), minimum_support, workers, result,
                               peak_rss_mb);
    return result;
}

}  // namespace ccbench