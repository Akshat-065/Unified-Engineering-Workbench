#include "thermal/solver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace workbench::thermal {
namespace {

void require_finite(double value, const char* name) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument(std::string(name) + " must be finite");
  }
}

void validate(const Parameters& parameters, std::size_t element_count) {
  require_finite(parameters.length_m, "length");
  require_finite(parameters.area_m2, "area");
  require_finite(parameters.conductivity_w_per_m_k, "conductivity");
  require_finite(parameters.heat_generation_w_per_m3, "heat generation");
  require_finite(parameters.left_temperature_c, "left boundary temperature");
  require_finite(parameters.right_temperature_c, "right boundary temperature");
  if (parameters.length_m <= 0.0) throw std::invalid_argument("length must be positive");
  if (parameters.area_m2 <= 0.0) throw std::invalid_argument("area must be positive");
  if (parameters.conductivity_w_per_m_k <= 0.0) throw std::invalid_argument("conductivity must be positive");
  if (parameters.heat_generation_w_per_m3 <= 0.0) throw std::invalid_argument("heat generation must be positive");
  if (element_count == 0) throw std::invalid_argument("element count must be at least one");
  if (element_count == std::numeric_limits<std::size_t>::max()) {
    throw std::invalid_argument("element count is too large");
  }
}

std::vector<double> solve_dense_system(std::vector<std::vector<double>> matrix,
                                       std::vector<double> rhs) {
  const std::size_t size = rhs.size();
  for (std::size_t pivot_column = 0; pivot_column < size; ++pivot_column) {
    std::size_t pivot_row = pivot_column;
    for (std::size_t row = pivot_column + 1; row < size; ++row) {
      if (std::abs(matrix[row][pivot_column]) > std::abs(matrix[pivot_row][pivot_column])) {
        pivot_row = row;
      }
    }
    if (!std::isfinite(matrix[pivot_row][pivot_column]) ||
        std::abs(matrix[pivot_row][pivot_column]) <= std::numeric_limits<double>::epsilon()) {
      throw std::runtime_error("thermal stiffness system is singular or non-finite");
    }
    std::swap(matrix[pivot_column], matrix[pivot_row]);
    std::swap(rhs[pivot_column], rhs[pivot_row]);

    for (std::size_t row = pivot_column + 1; row < size; ++row) {
      const double factor = matrix[row][pivot_column] / matrix[pivot_column][pivot_column];
      matrix[row][pivot_column] = 0.0;
      for (std::size_t column = pivot_column + 1; column < size; ++column) {
        matrix[row][column] -= factor * matrix[pivot_column][column];
      }
      rhs[row] -= factor * rhs[pivot_column];
    }
  }

  std::vector<double> solution(size, 0.0);
  for (std::size_t reverse = size; reverse > 0; --reverse) {
    const std::size_t row = reverse - 1;
    double value = rhs[row];
    for (std::size_t column = row + 1; column < size; ++column) {
      value -= matrix[row][column] * solution[column];
    }
    solution[row] = value / matrix[row][row];
    if (!std::isfinite(solution[row])) {
      throw std::runtime_error("thermal solution contains a non-finite temperature");
    }
  }
  return solution;
}

PointResult make_point(const Parameters& parameters, double position_m, double finite_element_c) {
  const double exact_c = analytical_temperature(parameters, position_m);
  return {position_m, finite_element_c, exact_c, std::abs(finite_element_c - exact_c)};
}

}  // namespace

double analytical_temperature(const Parameters& parameters, double position_m) {
  require_finite(position_m, "position");
  if (!std::isfinite(parameters.length_m) || parameters.length_m <= 0.0 ||
      !std::isfinite(parameters.conductivity_w_per_m_k) || parameters.conductivity_w_per_m_k <= 0.0 ||
      !std::isfinite(parameters.heat_generation_w_per_m3) ||
      !std::isfinite(parameters.left_temperature_c) || !std::isfinite(parameters.right_temperature_c)) {
    throw std::invalid_argument("analytical solution parameters must be finite with positive length and conductivity");
  }
  if (position_m < 0.0 || position_m > parameters.length_m) {
    throw std::invalid_argument("position must lie within the bar");
  }
  const double x = position_m;
  const double linear = parameters.left_temperature_c +
      (parameters.right_temperature_c - parameters.left_temperature_c) * x / parameters.length_m;
  const double generation = parameters.heat_generation_w_per_m3 /
      (2.0 * parameters.conductivity_w_per_m_k) * x * (parameters.length_m - x);
  const double temperature = linear + generation;
  if (!std::isfinite(temperature)) throw std::invalid_argument("analytical temperature is non-finite");
  return temperature;
}

MeshResult solve(const Parameters& parameters, std::size_t element_count) {
  validate(parameters, element_count);
  const std::size_t node_count = element_count + 1;
  const double element_length = parameters.length_m / static_cast<double>(element_count);
  const std::size_t interior_count = element_count - 1;

  std::vector<std::vector<double>> global_stiffness(
      node_count, std::vector<double>(node_count, 0.0));
  std::vector<double> global_load(node_count, 0.0);

  const double diagonal = parameters.conductivity_w_per_m_k * parameters.area_m2 / element_length;
  const double off_diagonal = -diagonal;
  const double element_load = parameters.heat_generation_w_per_m3 * parameters.area_m2 * element_length / 2.0;
  for (std::size_t element = 0; element < element_count; ++element) {
    const std::size_t left = element;
    const std::size_t right = element + 1;
    global_stiffness[left][left] += diagonal;
    global_stiffness[left][right] += off_diagonal;
    global_stiffness[right][left] += off_diagonal;
    global_stiffness[right][right] += diagonal;
    global_load[left] += element_load;
    global_load[right] += element_load;
  }

  std::vector<double> nodal_temperatures(node_count, 0.0);
  nodal_temperatures.front() = parameters.left_temperature_c;
  nodal_temperatures.back() = parameters.right_temperature_c;
  if (interior_count > 0) {
    std::vector<std::vector<double>> reduced_stiffness(
        interior_count, std::vector<double>(interior_count, 0.0));
    std::vector<double> reduced_load(interior_count, 0.0);
    for (std::size_t local_row = 0; local_row < interior_count; ++local_row) {
      const std::size_t global_row = local_row + 1;
      double rhs = global_load[global_row];
      rhs -= global_stiffness[global_row][0] * parameters.left_temperature_c;
      rhs -= global_stiffness[global_row][element_count] * parameters.right_temperature_c;
      reduced_load[local_row] = rhs;
      for (std::size_t local_column = 0; local_column < interior_count; ++local_column) {
        reduced_stiffness[local_row][local_column] =
            global_stiffness[global_row][local_column + 1];
      }
    }
    const auto interior = solve_dense_system(std::move(reduced_stiffness), std::move(reduced_load));
    std::copy(interior.begin(), interior.end(), nodal_temperatures.begin() + 1);
  }

  MeshResult result{element_count, nodal_temperatures, {}, {}, 0.0};
  result.nodes.reserve(node_count);
  result.midpoints.reserve(element_count);
  for (std::size_t node = 0; node < node_count; ++node) {
    const double x = static_cast<double>(node) * element_length;
    result.nodes.push_back(make_point(parameters, x, nodal_temperatures[node]));
  }
  for (std::size_t element = 0; element < element_count; ++element) {
    const double x = (static_cast<double>(element) + 0.5) * element_length;
    const double interpolated = (nodal_temperatures[element] + nodal_temperatures[element + 1]) / 2.0;
    const auto point = make_point(parameters, x, interpolated);
    result.maximum_midpoint_error_c = std::max(result.maximum_midpoint_error_c, point.absolute_error_c);
    result.midpoints.push_back(point);
  }
  return result;
}

}  // namespace workbench::thermal
