#include "thermal/results.h"
#include "thermal/solver.h"

#include <array>
#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
  try {
    const std::filesystem::path output_directory = argc > 1 ? argv[1] : "results";
    if (argc > 2) {
      std::cerr << "Usage: workbench_thermal [output-directory]\n";
      return 2;
    }
    const workbench::thermal::Parameters parameters;
    constexpr std::array<std::size_t, 4> mesh_sizes{10, 20, 40, 80};
    std::vector<workbench::thermal::MeshResult> results;
    results.reserve(mesh_sizes.size());
    for (const auto elements : mesh_sizes) {
      results.push_back(workbench::thermal::solve(parameters, elements));
      const auto& result = results.back();
      std::cout << "N=" << elements << " nodes=" << result.nodal_temperatures_c.size()
                << " max midpoint absolute error=" << result.maximum_midpoint_error_c << " C\n";
    }
    workbench::thermal::write_results(results, output_directory);
    std::cout << "CSV results written to " << output_directory.string() << '\n';
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Thermal analysis failed: " << error.what() << '\n';
    return 1;
  }
}
