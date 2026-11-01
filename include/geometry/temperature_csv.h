#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace workbench::geometry {

struct TemperatureNode {
  std::int64_t node_id{};
  double x_m{}, y_m{}, z_m{}, temperature_C{};
};
struct TemperaturePointCloud { std::vector<TemperatureNode> nodes; };

// Requires exactly: node_id,x_m,y_m,z_m,temperature_C
TemperaturePointCloud parse_temperature_csv(const std::string& contents);

}  // namespace workbench::geometry
