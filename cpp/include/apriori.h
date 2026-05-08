#pragma once

#include "common.h"

namespace ccn2 {

MiningResult apriori_original(const Dataset& dataset, double min_sup, unsigned workers = 0,
                              double* peak_rss_mb = nullptr);

MiningResult apriori_optimized(Dataset dataset, double min_sup, unsigned workers = 0,
                               double* peak_rss_mb = nullptr);

}  // namespace ccn2