#pragma once

#include "common.h"

#include <filesystem>

namespace ccbench {

Dataset load_dataset(const std::filesystem::path& file_path);

}  // namespace ccbench
