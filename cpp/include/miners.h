#pragma once

#include "common.h"

namespace ccn2 {

// Apriori original implementation.
// Parameters:
// - dataset: transactions (vector of transactions)
// - min_sup: minimum support threshold in [0,1]
// - workers: number of threads to use for parallel counting (0 => auto)
// - peak_rss_mb: optional out parameter to receive peak RSS memory measurement
MiningResult apriori_original(const Dataset& dataset, double min_sup, unsigned workers = 0,
							 double* peak_rss_mb = nullptr);

// Apriori with dataset filtering/optimization.
MiningResult apriori_optimized(Dataset dataset, double min_sup, unsigned workers = 0,
							  double* peak_rss_mb = nullptr);

// Linear-table (projection-based) miner. Typically better for large datasets and
// low min_sup thresholds.
MiningResult linear_table_mine(const Dataset& dataset, double min_sup, double* peak_rss_mb = nullptr);

}  // namespace ccn2
