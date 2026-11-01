#include "thermal/solver.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace workbench::thermal {
namespace {

TEST(ThermalSolver, EnforcesBothPrescribedBoundaryTemperatures) {
  const auto result = solve(Parameters{}, 10);
  EXPECT_DOUBLE_EQ(result.nodal_temperatures_c.front(), 100.0);
  EXPECT_DOUBLE_EQ(result.nodal_temperatures_c.back(), 20.0);
}

TEST(ThermalSolver, RejectsZeroAndNegativePhysicalInputs) {
  const auto invalid = [](Parameters parameters) {
    EXPECT_THROW(solve(parameters, 10), std::invalid_argument);
  };
  auto parameters = Parameters{};
  parameters.length_m = 0.0; invalid(parameters);
  parameters = Parameters{}; parameters.length_m = -1.0; invalid(parameters);
  parameters = Parameters{}; parameters.area_m2 = 0.0; invalid(parameters);
  parameters = Parameters{}; parameters.area_m2 = -1.0; invalid(parameters);
  parameters = Parameters{}; parameters.conductivity_w_per_m_k = 0.0; invalid(parameters);
  parameters = Parameters{}; parameters.conductivity_w_per_m_k = -1.0; invalid(parameters);
  parameters = Parameters{}; parameters.heat_generation_w_per_m3 = 0.0; invalid(parameters);
  parameters = Parameters{}; parameters.heat_generation_w_per_m3 = -1.0; invalid(parameters);
  parameters = Parameters{}; parameters.conductivity_w_per_m_k = std::numeric_limits<double>::infinity(); invalid(parameters);
  parameters = Parameters{}; parameters.heat_generation_w_per_m3 = std::numeric_limits<double>::quiet_NaN(); invalid(parameters);
  parameters = Parameters{}; parameters.left_temperature_c = std::numeric_limits<double>::infinity(); invalid(parameters);
}

TEST(ThermalSolver, RejectsAnEmptyMesh) {
  EXPECT_THROW(solve(Parameters{}, 0), std::invalid_argument);
  EXPECT_THROW(analytical_temperature(Parameters{}, std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
}

TEST(ThermalSolver, ProducesElementCountPlusOneNodes) {
  for (const std::size_t elements : {10U, 20U, 40U, 80U}) {
    const auto result = solve(Parameters{}, elements);
    EXPECT_EQ(result.nodal_temperatures_c.size(), elements + 1);
    EXPECT_EQ(result.nodes.size(), elements + 1);
    EXPECT_EQ(result.midpoints.size(), elements);
  }
}

TEST(ThermalSolver, NodalSolutionMatchesTheAnalyticalQuadratic) {
  const Parameters parameters;
  const auto result = solve(parameters, 10);
  for (const auto& node : result.nodes) {
    EXPECT_NEAR(node.finite_element_temperature_c, node.analytical_temperature_c, 1e-10);
  }
  EXPECT_NEAR(analytical_temperature(parameters, 0.5), 185.0, 1e-12);
}

TEST(ThermalSolver, MidpointErrorMatchesTheInterpolationError) {
  const Parameters parameters;
  const auto result = solve(parameters, 10);
  const double element_length = parameters.length_m / 10.0;
  const double expected_error = parameters.heat_generation_w_per_m3 * element_length * element_length /
      (8.0 * parameters.conductivity_w_per_m_k);
  EXPECT_NEAR(result.maximum_midpoint_error_c, expected_error, 1e-11);
}

TEST(ThermalSolver, ErrorDecreasesQuadraticallyUnderMeshRefinement) {
  constexpr std::array<std::size_t, 4> elements{10, 20, 40, 80};
  double previous_error = std::numeric_limits<double>::infinity();
  for (const auto count : elements) {
    const auto result = solve(Parameters{}, count);
    EXPECT_LT(result.maximum_midpoint_error_c, previous_error);
    if (std::isfinite(previous_error)) {
      EXPECT_NEAR(previous_error / result.maximum_midpoint_error_c, 4.0, 1e-9);
    }
    previous_error = result.maximum_midpoint_error_c;
  }
}

}  // namespace
}  // namespace workbench::thermal
