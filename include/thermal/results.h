#pragma once

#include "thermal/solver.h"

#include <filesystem>
#include <vector>

namespace workbench::thermal {

void write_results(const std::vector<MeshResult>& meshes, const std::filesystem::path& output_directory);

}  // namespace workbench::thermal
