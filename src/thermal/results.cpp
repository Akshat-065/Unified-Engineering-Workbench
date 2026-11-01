#include "thermal/results.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace workbench::thermal {

void write_results(const std::vector<MeshResult>& meshes, const std::filesystem::path& output_directory) {
  std::filesystem::create_directories(output_directory);
  std::ofstream values(output_directory / "thermal_results.csv", std::ios::trunc);
  std::ofstream convergence(output_directory / "thermal_convergence.csv", std::ios::trunc);
  if (!values || !convergence) throw std::runtime_error("unable to open thermal CSV output files");
  values << "mesh_elements,point_type,point_index,x_m,finite_element_temperature_C,analytical_temperature_C,absolute_error_C\n";
  convergence << "mesh_elements,max_absolute_midpoint_error_C\n";
  values << std::setprecision(17);
  convergence << std::setprecision(17);
  for (const auto& mesh : meshes) {
    for (std::size_t index = 0; index < mesh.nodes.size(); ++index) {
      const auto& point = mesh.nodes[index];
      values << mesh.element_count << ",node," << index << ',' << point.position_m << ','
             << point.finite_element_temperature_c << ',' << point.analytical_temperature_c << ','
             << point.absolute_error_c << '\n';
    }
    for (std::size_t index = 0; index < mesh.midpoints.size(); ++index) {
      const auto& point = mesh.midpoints[index];
      values << mesh.element_count << ",midpoint," << index << ',' << point.position_m << ','
             << point.finite_element_temperature_c << ',' << point.analytical_temperature_c << ','
             << point.absolute_error_c << '\n';
    }
    convergence << mesh.element_count << ',' << mesh.maximum_midpoint_error_c << '\n';
  }
  values.flush();
  convergence.flush();
  if (!values || !convergence) throw std::runtime_error("failed while writing thermal CSV output");
}

}  // namespace workbench::thermal
