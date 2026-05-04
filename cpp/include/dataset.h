#pragma once

#include "common.h"

#include <filesystem>

namespace ccn2 {

Dataset load_dataset(const std::filesystem::path& file_path);

}  // namespace ccn2
