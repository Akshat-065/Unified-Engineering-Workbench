#include "geometry/temperature_csv.h"
#include "geometry/ascii_stl.h"

#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace workbench::geometry {
namespace {
std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> fields; std::size_t start = 0;
  for (;;) { const auto comma = line.find(',', start); fields.push_back(line.substr(start, comma == std::string::npos ? comma : comma - start)); if (comma == std::string::npos) break; start = comma + 1; }
  return fields;
}
double parse_double(const std::string& value, std::size_t row, const char* field) {
  std::size_t used = 0; double result{};
  try { result = std::stod(value, &used); } catch (...) { throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": invalid " + field + " value"); }
  if (used != value.size() || !std::isfinite(result)) throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": " + field + " must be finite");
  return result;
}
}
TemperaturePointCloud parse_temperature_csv(const std::string& contents) {
  if (contents.empty() || contents.find_first_not_of(" \t\r\n") == std::string::npos) throw std::runtime_error("Temperature CSV is empty");
  if (contents.size() > kMaxGeometryUploadBytes) throw std::runtime_error("Temperature CSV exceeds the 16 MiB upload limit");
  std::istringstream input(contents); std::string line; std::size_t row = 0;
  if (!std::getline(input, line)) throw std::runtime_error("Temperature CSV is empty"); ++row;
  if (!line.empty() && line.back() == '\r') line.pop_back();
  if (split(line) != std::vector<std::string>{"node_id", "x_m", "y_m", "z_m", "temperature_C"}) throw std::runtime_error("Temperature CSV header must be exactly node_id,x_m,y_m,z_m,temperature_C");
  TemperaturePointCloud cloud; std::set<std::int64_t> ids;
  while (std::getline(input, line)) {
    ++row; if (!line.empty() && line.back() == '\r') line.pop_back();
    const auto fields = split(line);
    if (fields.size() != 5) throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": expected exactly five comma-separated fields");
    std::size_t used = 0; std::int64_t id{};
    try { id = std::stoll(fields[0], &used); } catch (...) { throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": node_id must be an integer"); }
    if (used != fields[0].size()) throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": node_id must be an integer");
    if (!ids.insert(id).second) throw std::runtime_error("Temperature CSV row " + std::to_string(row) + ": duplicate node_id " + fields[0]);
    cloud.nodes.push_back({id, parse_double(fields[1], row, "x_m"), parse_double(fields[2], row, "y_m"), parse_double(fields[3], row, "z_m"), parse_double(fields[4], row, "temperature_C")});
  }
  if (cloud.nodes.empty()) throw std::runtime_error("Temperature CSV contains a header but no data rows");
  return cloud;
}
}  // namespace workbench::geometry
