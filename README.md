# Unified Engineering Workbench

Unified Engineering Workbench for the PASS SCIENCE WORKS R&D Internship 2026–2027 common assignment. The project provides a React/Vite engineering dashboard integrated with C++17 APIs and numerical modules for geometry validation, thermal FEA, parallel benchmarks, simulated hardware adapters, and hand-derived backpropagation.

## Requirements

- Windows 11 x64 and Visual Studio 18 2026 Build Tools with the **Desktop development with C++** workload (MSVC and Windows SDK).
- CMake 4.2 or newer for the Visual Studio 18 generator.
- Node.js 20 or newer with npm.
- Git and network access for the first CMake configure (CMake FetchContent downloads pinned dependencies).
- Python 3 with Matplotlib for rendering thermal plots from the solver CSV output (`python -m pip install -r requirements.txt`).

The thermal plotter uses Python's standard-library `argparse`, `csv`, and `pathlib` modules plus `matplotlib.pyplot`, pinned as `matplotlib==3.10.8` in the root `requirements.txt`. The Task F plotter emits SVG using only the Python standard library. Plotters only visualize CSV output from C++; numerical solving and CSV generation remain in C++.

### C++ dependencies

| Dependency | Version | Purpose | Acquisition |
| --- | --- | --- | --- |
| cpp-httplib | 0.18.3 | Lightweight HTTP server | CMake FetchContent |
| nlohmann/json | 3.12.0 | JSON serialization | CMake FetchContent |
| GoogleTest | 1.15.2 | C++ unit tests | CMake FetchContent |

The C++ core is a separate `workbench_core` library; the HTTP server is an executable that links the core. The dashboard uses React 18.3.1, Vite 6.4.5, and lucide-react 0.468.0, pinned in `frontend/package.json` and `frontend/package-lock.json`.

## Thermal analysis (Part 2)

Build the C++ solver using the same CMake preset above, then run it from the repository root. The executable performs all four numerical mesh cases and writes the actual computed nodal/midpoint results and convergence values:

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64
ctest --test-dir build/vs18-x64 -C Release --output-on-failure
& .\build\vs18-x64\Release\workbench_thermal.exe .\results
Import-Csv .\results\thermal_convergence.csv | Format-Table
python -m pip install -r requirements.txt
python scripts/plot_thermal_results.py --results .\results
```

The solver runs uniform meshes with 10, 20, 40, and 80 elements. It writes `results/thermal_results.csv` (nodal and element-midpoint values) and `results/thermal_convergence.csv` (maximum absolute midpoint errors). The plotting script reads those C++-generated CSVs and writes labelled temperature, per-mesh analytical comparison, and convergence PNG plots in `results/`. Supply an alternate output directory to the solver and plotting script with `workbench_thermal <directory>` and `--results <directory>` respectively.

Thermal implementation, equations, boundary treatment, validation, assumptions, and the interview explanation are documented in [docs/thermal-analysis.md](docs/thermal-analysis.md).

## Build and run on Windows

Open a Visual Studio 18 2026 x64 Developer PowerShell (or another terminal where CMake can locate the VS 18 Build Tools):

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64
ctest --test-dir build/vs18-x64 -C Release --output-on-failure
```

The Windows preset uses the `Visual Studio 18 2026` generator and the existing `build/vs18-x64` directory. The pinned cpp-httplib 0.18.3, nlohmann/json 3.12.0, and (when tests are enabled) GoogleTest 1.15.2 releases are fetched as SHA-256-verified source archives. This avoids Git clone negotiation and reuses matching populated FetchContent source directories. CMake reports whether an exact-version installed package or a local source directory is used, or when a verified archive must be downloaded. An incomplete dependency directory is not considered available.

Preinstalled exact-version CMake packages are preferred. Install them to a prefix and pass it explicitly, for example:

```powershell
cmake --preset windows-msvc-x64 -DCMAKE_PREFIX_PATH="C:/engineering/dependencies"
cmake --build --preset windows-msvc-x64
ctest --test-dir build/vs18-x64 -C Release --output-on-failure
```

Alternatively, use CMake's `FETCHCONTENT_SOURCE_DIR_HTTPLIB`, `FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON`, and `FETCHCONTENT_SOURCE_DIR_GOOGLETEST` cache options to point at already populated, matching source trees. Do not set `FETCHCONTENT_FULLY_DISCONNECTED=ON` until all required sources or packages have been verified.

Run the backend in one terminal:

```powershell
& .\build\vs18-x64\Release\workbench_server.exe
```

Run the frontend in another:

```powershell
cd frontend
npm ci
npm run dev
```

Open the local URL printed by Vite (usually `http://localhost:5173`). Vite proxies `/api` requests to the local backend at `http://127.0.0.1:8080`. To change the backend port, set `WORKBENCH_PORT` and update the proxy in `frontend/vite.config.js` to match.

The API includes `GET /api/health`, `GET /api/status`, `POST /api/thermal/solve`, geometry upload routes, performance benchmark routes, simulated hardware routes, and `GET /api/ml/backprop`. The dashboard shows backend and host status, module navigation, and recent operations. See [docs/react-integration.md](docs/react-integration.md) for page/API mapping, exports, and safe cancellation behavior.

## Repository layout

`frontend/` contains the React UI. C++ public interfaces are under `include/`, implementation under `src/`, and `tests/` contains GoogleTest suites. Each completed engineering module is integrated through the C++ API; React handles user interaction, visualization, and API communication only.

The Geometry Viewer accepts validated ASCII STL and temperature CSV uploads through the C++ API. See [docs/geometry-and-temperature-viewers.md](docs/geometry-and-temperature-viewers.md) for formats, size limits, example-data provenance, and tests. Binary STL, native CAD solids, and mesh-to-result registration are not supported.

Task D adds reproducible serial/CPU-parallel thermal batch benchmarking with a fixed-seed input generator and raw CSV output. See [docs/task-d-performance.md](docs/task-d-performance.md) for worker-pool details, resource limits, measurement definitions, and commands.

Task E adds a simulated host-connected hardware dashboard and C++ device-adapter interface for the Pico 2, Arty A7-35T and J-Link EDU Mini assumptions. It does not communicate with physical hardware. See [docs/hardware-integration.md](docs/hardware-integration.md) for the labelled block diagram, pin-voltage/interface table, safety notes and failure-recovery example.

Task F adds a hand-derived single-sample backpropagation implementation in the C++17 core, central finite-difference checks, 100 additional training steps after the required initial update, a `/api/ml/backprop` endpoint, and the Backpropagation dashboard. Generate `results/backpropagation_loss.csv` with `workbench_ml`; use `python scripts/plot_backpropagation_loss.py` to render its SVG plot. See [docs/task-f-backpropagation.md](docs/task-f-backpropagation.md) for the full derivation, verified run values, and handwritten preparation guide. The typed derivation is a study guide and is not handwritten evidence.
