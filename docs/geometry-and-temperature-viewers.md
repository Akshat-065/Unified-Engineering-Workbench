# Geometry and temperature viewers

## Supported inputs

The C++ API parses ASCII STL facet records and returns solid name, facet normals, and three vertices per triangle. Files must have `solid`, structurally complete `facet normal` / `outer loop` / exactly three `vertex` / `endloop` / `endfacet` records, and a closing `endsolid`. Coordinates and normals must be finite, and triangle vertices must not be collinear. STL has no reliable units field; the viewer labels units as unspecified. Binary STL, native CAD solids, and mesh-to-result registration are not supported.

Temperature results require this exact CSV header:

```text
node_id,x_m,y_m,z_m,temperature_C
```

Rows have five comma-separated fields. Node IDs are signed 64-bit integers and unique within the file. Coordinates use metres and temperature uses degrees Celsius. Every numeric value must be finite. Blank or malformed records, empty data sets, duplicate IDs, and non-finite values are errors.

## Viewer and API

The React/Three.js page posts raw file bytes to `POST /api/geometry/stl` or `POST /api/geometry/temperature`. Parsing and validation happen only in C++. A successful STL response includes all expanded triangle positions and normals; a CSV response includes all validated nodes and temperature range. A parser error is returned as JSON with HTTP 400 and an actionable message. Request bodies are limited to 16 MiB per file both at the HTTP server and in each parser. Files above the limit are rejected (cpp-httplib can reject oversized requests before the API handler returns a parser JSON response).

## File-size limit and test method

The 16 MiB cap is an explicit request-memory policy for a local interactive upload endpoint; it is not a measured throughput or benchmark limit. It keeps a single request body bounded while allowing small and medium engineering exports. The C++ tests create an otherwise valid ASCII STL padded with legal whitespace to exactly 16 MiB and check it parses, then append one byte and check rejection. A CSV larger than 16 MiB is also checked for rejection. The limit applies to the full file bytes, not just parsed payload.

## Example data and provenance

- `data/examples/unit_tetrahedron_ascii.stl` is a small hand-authored, self-created ASCII STL tetrahedron. It is illustrative geometry, has no source CAD file, and declares no physical units.
- `data/examples/synthetic_temperature_points.csv` is self-created synthetic data with eight points on a 0.1 m grid. Values are illustrative degrees Celsius only. This is **not an actual ANSYS export**. The schema is the exact header shown above; coordinates are metres and temperatures are °C.

## Tests

`workbench_geometry_tests` covers a valid ASCII STL, empty/no-facet/malformed STL, non-finite coordinates, degenerate triangles, the STL size boundary, exact CSV schema/value handling, empty/header-only CSV, malformed rows, NaN, duplicate IDs, and oversized CSV. Build with `cmake --build build/vs18-x64 --config Release --target workbench_geometry_tests` and run with `ctest --test-dir build/vs18-x64 -C Release -R "AsciiStl|TemperatureCsv" --output-on-failure`.
