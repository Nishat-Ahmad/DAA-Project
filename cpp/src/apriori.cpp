#include "apriori.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <thread>
#include <unordered_set>

namespace ccbench {

namespace {

bool prefix_equal(const Itemset& lhs, const Itemset& rhs, std::size_t prefix_length) {
    return std::equal(lhs.begin(), lhs.begin() + static_cast<std::ptrdiff_t>(prefix_length), rhs.begin());
}

std::vector<Itemset> collect_itemsets_from_map(const CountMap& counts) {
    std::vector<Itemset> itemsets;
    itemsets.reserve(counts.size());
    for (const auto& [encoded, _] : counts) {
        Itemset itemset;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= encoded.length(); ++i) {
            if (i == encoded.length() || encoded[i] == ',') {
                if (i > start) {
                    itemset.push_back(std::stoi(encoded.substr(start, i - start)));
                }
                start = i + 1;
            }
        }
        itemsets.push_back(std::move(itemset));
    }
    std::sort(itemsets.begin(), itemsets.end());
    return itemsets;
}

std::vector<Itemset> generate_candidates(const std::vector<Itemset>& prev_frequents, std::size_t k) {
    std::vector<Itemset> candidates;
    std::unordered_set<std::string> seen;

    std::unordered_set<std::string> prev_lookup;
    prev_lookup.reserve(prev_frequents.size());
    for (const auto& itemset : prev_frequents) {
        prev_lookup.insert(encode_itemset(itemset));
    }

    for (std::size_t i = 0; i < prev_frequents.size(); ++i) {
        for (std::size_t j = i + 1; j < prev_frequents.size(); ++j) {
            const auto& lhs = prev_frequents[i];
            const auto& rhs = prev_frequents[j];
            if (!prefix_equal(lhs, rhs, k - 2)) {
                continue;
            }

            Itemset candidate = lhs;
            candidate.push_back(rhs.back());

            bool valid = true;
            std::string subset_encoded;
            for (std::size_t remove_index = 0; remove_index < candidate.size(); ++remove_index) {
                subset_encoded.clear();
                for (std::size_t idx = 0; idx < candidate.size(); ++idx) {
                    if (idx != remove_index) {
                        if (!subset_encoded.empty()) {
                            subset_encoded.push_back(',');
                        }
                        subset_encoded += std::to_string(candidate[idx]);
                    }
                }
                if (prev_lookup.find(subset_encoded) == prev_lookup.end()) {
                    valid = false;
                    break;
                }
            }

            if (!valid) {
                continue;
            }

            const auto encoded = encode_itemset(candidate);
            if (seen.insert(encoded).second) {
                candidates.push_back(std::move(candidate));
            }
        }
    }

    return candidates;
}

std::vector<int> count_supports_parallel(const Dataset& dataset, const std::vector<Itemset>& candidates,
                                         unsigned workers) {
    std::vector<int> counts(candidates.size(), 0);
    if (candidates.empty() || dataset.empty()) {
        return counts;
    }

    unsigned thread_count = workers == 0 ? std::thread::hardware_concurrency() : workers;
    if (thread_count == 0) {
        thread_count = 1;
    }
    thread_count = static_cast<unsigned>(std::min<std::size_t>(thread_count, dataset.size()));

    if (thread_count <= 1 || candidates.size() < 32 || dataset.size() < 256) {
        for (const auto& transaction : dataset) {
            for (std::size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
                if (std::includes(transaction.begin(), transaction.end(), candidates[candidate_index].begin(),
                                  candidates[candidate_index].end())) {
                    ++counts[candidate_index];
                }
            }
        }
        return counts;
    }

    std::vector<std::vector<int>> local_counts(thread_count, std::vector<int>(candidates.size(), 0));
    std::vector<std::thread> pool;
    pool.reserve(thread_count);

    const std::size_t chunk_size = (dataset.size() + thread_count - 1) / thread_count;
    for (unsigned thread_index = 0; thread_index < thread_count; ++thread_index) {
        const std::size_t begin = thread_index * chunk_size;
        const std::size_t end = std::min(dataset.size(), begin + chunk_size);
        if (begin >= end) {
            break;
        }

        pool.emplace_back([thread_index, begin, end, &dataset, &candidates, &local_counts]() {
            auto& local = local_counts[thread_index];
            for (std::size_t row_index = begin; row_index < end; ++row_index) {
                const auto& transaction = dataset[row_index];
                for (std::size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
                    if (std::includes(transaction.begin(), transaction.end(), candidates[candidate_index].begin(),
                                      candidates[candidate_index].end())) {
                        ++local[candidate_index];
                    }
                }
            }
        });
    }

    for (auto& worker : pool) {
        worker.join();
    }

    for (const auto& local : local_counts) {
        for (std::size_t candidate_index = 0; candidate_index < counts.size(); ++candidate_index) {
            counts[candidate_index] += local[candidate_index];
        }
    }

    return counts;
}

std::vector<int> collect_sorted_frequent_items(const std::vector<Itemset>& frequent_itemsets) {
    std::unordered_set<int> unique_items;
    for (const auto& itemset : frequent_itemsets) {
        unique_items.insert(itemset.begin(), itemset.end());
    }
    std::vector<int> items(unique_items.begin(), unique_items.end());
    std::sort(items.begin(), items.end());
    return items;
}

MiningResult frequent_singletons(const Dataset& dataset, double min_sup, double* peak_rss_mb) {
    MiningResult result;
    std::unordered_map<int, std::size_t> counts;
    for (const auto& transaction : dataset) {
        for (int item : transaction) {
            ++counts[item];
        }
    }

    const std::size_t minimum_support = static_cast<std::size_t>(std::ceil(min_sup * static_cast<double>(dataset.size())));

    std::vector<std::pair<int, std::size_t>> sorted_counts(counts.begin(), counts.end());
    std::sort(sorted_counts.begin(), sorted_counts.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });

    for (const auto& [item, count] : sorted_counts) {
        if (count >= minimum_support) {
            result.frequent_itemsets[encode_itemset(Itemset{item})] = static_cast<int>(count);
        }
    }

    update_peak(peak_rss_mb);
    return result;
}

Dataset filter_dataset_by_items(const Dataset& dataset, const std::vector<int>& frequent_items) {
    Dataset filtered_dataset;
    filtered_dataset.reserve(dataset.size());

    for (const auto& transaction : dataset) {
        Transaction filtered;
        filtered.reserve(transaction.size());
        std::set_intersection(transaction.begin(), transaction.end(), frequent_items.begin(), frequent_items.end(),
                              std::back_inserter(filtered));
        if (!filtered.empty()) {
            filtered_dataset.push_back(std::move(filtered));
        }
    }

    return filtered_dataset;
}

}  // namespace

MiningResult apriori_original(const Dataset& dataset, double min_sup, unsigned workers, double* peak_rss_mb) {
    MiningResult result;
    if (dataset.empty()) {
        return result;
    }

    const std::size_t minimum_support = static_cast<std::size_t>(std::ceil(min_sup * static_cast<double>(dataset.size())));

    MiningResult singleton_result = frequent_singletons(dataset, min_sup, peak_rss_mb);
    result.frequent_itemsets = std::move(singleton_result.frequent_itemsets);
    std::vector<Itemset> current_frequents = collect_itemsets_from_map(result.frequent_itemsets);

    std::size_t k = 2;
    while (!current_frequents.empty()) {
        std::sort(current_frequents.begin(), current_frequents.end());
        const auto candidates = generate_candidates(current_frequents, k);
        if (candidates.empty()) {
            break;
        }

        result.candidate_count += candidates.size();
        const auto counts = count_supports_parallel(dataset, candidates, workers);

        std::vector<Itemset> next_frequents;
        next_frequents.reserve(candidates.size());
        for (std::size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
            if (counts[candidate_index] >= static_cast<int>(minimum_support)) {
                next_frequents.push_back(candidates[candidate_index]);
                result.frequent_itemsets[encode_itemset(candidates[candidate_index])] = counts[candidate_index];
            }
        }

        update_peak(peak_rss_mb);
        current_frequents = std::move(next_frequents);
        ++k;
    }

    return result;
}

MiningResult apriori_optimized(Dataset dataset, double min_sup, unsigned workers, double* peak_rss_mb) {
    MiningResult result;
    if (dataset.empty()) {
        return result;
    }

    const std::size_t minimum_support = static_cast<std::size_t>(std::ceil(min_sup * static_cast<double>(dataset.size())));

    MiningResult singleton_result = frequent_singletons(dataset, min_sup, peak_rss_mb);
    result.frequent_itemsets = std::move(singleton_result.frequent_itemsets);

    std::vector<Itemset> current_frequents = collect_itemsets_from_map(result.frequent_itemsets);

    std::size_t k = 2;
    while (!current_frequents.empty()) {
        std::sort(current_frequents.begin(), current_frequents.end());
        const auto candidates = generate_candidates(current_frequents, k);
        if (candidates.empty()) {
            break;
        }

        result.candidate_count += candidates.size();

        std::vector<int> frequent_items = collect_sorted_frequent_items(current_frequents);
        dataset = filter_dataset_by_items(dataset, frequent_items);
        update_peak(peak_rss_mb);

        const auto counts = count_supports_parallel(dataset, candidates, workers);

        std::vector<Itemset> next_frequents;
        next_frequents.reserve(candidates.size());
        for (std::size_t candidate_index = 0; candidate_index < candidates.size(); ++candidate_index) {
            if (counts[candidate_index] >= static_cast<int>(minimum_support)) {
                next_frequents.push_back(candidates[candidate_index]);
                result.frequent_itemsets[encode_itemset(candidates[candidate_index])] = counts[candidate_index];
            }
        }

        update_peak(peak_rss_mb);
        current_frequents = std::move(next_frequents);
        ++k;
    }

    return result;
}

}  // namespace ccbench