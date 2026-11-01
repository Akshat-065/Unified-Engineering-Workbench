#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace workbench::geometry {

inline constexpr std::size_t kMaxGeometryUploadBytes = 16u * 1024u * 1024u;

struct Vec3 { double x{}, y{}, z{}; };
struct Triangle { Vec3 normal; Vec3 vertices[3]; };
struct StlModel { std::string solid_name; std::vector<Triangle> triangles; };

// Parses ASCII STL only. The byte cap applies to each uploaded file.
StlModel parse_ascii_stl(const std::string& contents);

}  // namespace workbench::geometry
