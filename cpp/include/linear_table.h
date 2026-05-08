#pragma once

#include "common.h"

namespace ccbench {

MiningResult linear_table_mine(const Dataset& dataset, double min_sup, double* peak_rss_mb = nullptr);

}  // namespace ccbench