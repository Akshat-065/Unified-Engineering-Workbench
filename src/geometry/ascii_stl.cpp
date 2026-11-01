#include "geometry/ascii_stl.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>

namespace workbench::geometry {
namespace {
std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r");
  if (first == std::string::npos) return {};
  return value.substr(first, value.find_last_not_of(" \t\r") - first + 1);
}
std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
std::vector<std::string> tokens(const std::string& line) {
  std::istringstream input(line); std::vector<std::string> out; std::string token;
  while (input >> token) out.push_back(token); return out;
}
double number(const std::string& value, std::size_t line, const char* what) {
  std::size_t used = 0; double result{};
  try { result = std::stod(value, &used); } catch (...) { throw std::runtime_error("ASCII STL line " + std::to_string(line) + ": invalid " + what + " value '" + value + "'"); }
  if (used != value.size() || !std::isfinite(result)) throw std::runtime_error("ASCII STL line " + std::to_string(line) + ": non-finite or invalid " + what + " value");
  return result;
}
Vec3 vector3(const std::vector<std::string>& parts, std::size_t line, const char* kind) {
  if (parts.size() != 4) throw std::runtime_error("ASCII STL line " + std::to_string(line) + ": expected three " + kind + " coordinates");
  return {number(parts[1], line, kind), number(parts[2], line, kind), number(parts[3], line, kind)};
}
bool same(const std::vector<std::string>& actual, std::initializer_list<const char*> expected) {
  if (actual.size() != expected.size()) return false;
  std::size_t i = 0; for (const auto* token : expected) if (lower(actual[i++]) != token) return false;
  return true;
}
}

StlModel parse_ascii_stl(const std::string& contents) {
  if (contents.empty() || contents.find_first_not_of(" \t\r\n") == std::string::npos) throw std::runtime_error("ASCII STL is empty");
  if (contents.size() > kMaxGeometryUploadBytes) throw std::runtime_error("ASCII STL exceeds the 16 MiB upload limit");
  std::istringstream input(contents); std::string line; std::size_t line_no = 0;
  auto next = [&]() -> std::vector<std::string> {
    while (std::getline(input, line)) { ++line_no; line = trim(line); if (!line.empty()) return tokens(line); }
    return {};
  };
  auto first = next();
  if (first.empty() || lower(first[0]) != "solid") throw std::runtime_error("ASCII STL line 1: expected 'solid <name>'");
  StlModel model;
  const auto first_space = line.find_first_of(" \t");
  model.solid_name = first_space == std::string::npos ? std::string{} : trim(line.substr(first_space + 1));
  bool closed = false;
  while (true) {
    auto parts = next();
    if (parts.empty()) break;
    if (lower(parts[0]) == "endsolid") {
      if (parts.size() > 1) {
        std::string closing = line.substr(line.find_first_of(" \t") == std::string::npos ? line.size() : line.find_first_of(" \t") + 1);
        if (!model.solid_name.empty() && trim(closing) != model.solid_name) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": closing solid name does not match");
      }
      closed = true; break;
    }
    if (!(parts.size() == 5 && lower(parts[0]) == "facet" && lower(parts[1]) == "normal")) {
      throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected 'facet normal nx ny nz' or 'endsolid'");
    }
    if (parts.size() != 5) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected 'facet normal nx ny nz'");
    Triangle triangle; triangle.normal = {number(parts[2], line_no, "normal"), number(parts[3], line_no, "normal"), number(parts[4], line_no, "normal")};
    parts = next(); if (!same(parts, {"outer", "loop"})) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected 'outer loop'");
    for (int i = 0; i < 3; ++i) { parts = next(); if (parts.size() != 4 || lower(parts[0]) != "vertex") throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected exactly three 'vertex x y z' records"); triangle.vertices[i] = vector3(parts, line_no, "vertex"); }
    parts = next(); if (!same(parts, {"endloop"})) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected 'endloop' after three vertices");
    parts = next(); if (!same(parts, {"endfacet"})) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": expected 'endfacet'");
    const auto& a = triangle.vertices[0]; const auto& b = triangle.vertices[1]; const auto& c = triangle.vertices[2];
    const double ux=b.x-a.x, uy=b.y-a.y, uz=b.z-a.z, vx=c.x-a.x, vy=c.y-a.y, vz=c.z-a.z;
    const double cx=uy*vz-uz*vy, cy=uz*vx-ux*vz, cz=ux*vy-uy*vx;
    if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(cz) || (cx == 0 && cy == 0 && cz == 0)) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": triangle vertices are degenerate");
    model.triangles.push_back(triangle);
  }
  if (!closed) throw std::runtime_error("ASCII STL: missing 'endsolid' record");
  if (model.triangles.empty()) throw std::runtime_error("ASCII STL contains no facets");
  if (!next().empty()) throw std::runtime_error("ASCII STL line " + std::to_string(line_no) + ": unexpected content after 'endsolid'");
  return model;
}
}  // namespace workbench::geometry
