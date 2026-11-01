#pragma once

#include <cstddef>
#include <vector>

namespace workbench::thermal {

struct Parameters {
  double length_m = 1.0;
  double area_m2 = 1.0e-4;
  double conductivity_w_per_m_k = 10.0;
  double heat_generation_w_per_m3 = 10000.0;
  double left_temperature_c = 100.0;
  double right_temperature_c = 20.0;
};

struct PointResult {
  double position_m;
  double finite_element_temperature_c;
  double analytical_temperature_c;
  double absolute_error_c;
};

struct MeshResult {
  std::size_t element_count;
  std::vector<double> nodal_temperatures_c;
  std::vector<PointResult> nodes;
  std::vector<PointResult> midpoints;
  double maximum_midpoint_error_c;
};

// Exact solution for the stated constant-property, uniform-generation problem.
double analytical_temperature(const Parameters& parameters, double position_m);

// Solve with uniform two-node linear elements. Throws std::invalid_argument
// for invalid parameters or an element count smaller than one.
MeshResult solve(const Parameters& parameters, std::size_t element_count);

}  // namespace workbench::thermal
