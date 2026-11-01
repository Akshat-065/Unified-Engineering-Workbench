#include "geometry/ascii_stl.h"
#include "geometry/temperature_csv.h"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace workbench::geometry;
namespace {
const std::string kFacet = "solid sample\n facet normal 0 0 1\n outer loop\n vertex 0 0 0\n vertex 1 0 0\n vertex 0 1 0\n endloop\n endfacet\nendsolid sample\n";
}
TEST(AsciiStl, ParsesSolidAndFacet) { const auto m=parse_ascii_stl(kFacet); EXPECT_EQ(m.solid_name,"sample"); ASSERT_EQ(m.triangles.size(),1u); EXPECT_DOUBLE_EQ(m.triangles[0].vertices[1].x,1); }
TEST(AsciiStl, RejectsEmptyMalformedNonfiniteAndDegenerate) {
  EXPECT_THROW(parse_ascii_stl(""),std::runtime_error);
  EXPECT_THROW(parse_ascii_stl("solid no-facets\nendsolid no-facets\n"),std::runtime_error);
  EXPECT_THROW(parse_ascii_stl("solid a\nfacet normal 0 0 1\nouter loop\nvertex 0 0 nan\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid a\n"),std::runtime_error);
  EXPECT_THROW(parse_ascii_stl("solid a\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 2 0 0\nendloop\nendfacet\nendsolid a\n"),std::runtime_error);
  EXPECT_THROW(parse_ascii_stl("not stl"),std::runtime_error);
}
TEST(AsciiStl, EnforcesDocumentedUploadSizeLimit) {
  std::string exactly_limit=kFacet;
  exactly_limit.append(kMaxGeometryUploadBytes-exactly_limit.size(),' ');
  EXPECT_NO_THROW(parse_ascii_stl(exactly_limit));
  exactly_limit.push_back(' ');
  EXPECT_THROW(parse_ascii_stl(exactly_limit),std::runtime_error);
}
TEST(TemperatureCsv, ParsesExactSchemaAndValues) {
  const auto cloud=parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0,1,2,23.5\n");
  ASSERT_EQ(cloud.nodes.size(),1u); EXPECT_EQ(cloud.nodes[0].node_id,1); EXPECT_DOUBLE_EQ(cloud.nodes[0].temperature_C,23.5);
}
TEST(TemperatureCsv, RejectsBadHeadersRowsNumbersAndDuplicateIds) {
  EXPECT_THROW(parse_temperature_csv(""),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("id,x,y,z,t\n1,0,0,0,1\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0,0,0,NaN\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0,Infinity,0,20\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\nabc,0,0,0,20\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0,0,0,1\n1,1,1,1,2\n"),std::runtime_error);
  EXPECT_THROW(parse_temperature_csv("node_id,x_m,y_m,z_m,temperature_C\n1,0,0\n"),std::runtime_error);
}
TEST(TemperatureCsv, EnforcesDocumentedUploadSizeLimit) {
  std::string contents="node_id,x_m,y_m,z_m,temperature_C\n1,0,0,0,20\n";
  contents.append(kMaxGeometryUploadBytes-contents.size()+1,' ');
  EXPECT_THROW(parse_temperature_csv(contents),std::runtime_error);
}
