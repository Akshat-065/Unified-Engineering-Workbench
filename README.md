# Unified Engineering Workbench

Unified Engineering Workbench is a Windows x64 engineering dashboard for the PASS SCIENCE WORKS R&D Internship 2026–2027 common assignment. It brings together C++17 geometry parsing, one-dimensional steady thermal FEA, CPU-parallel thermal batches, a documented embedded reference design with simulated adapters, and manually differentiated neural-network backpropagation.

## Submission package and one-click launch

Extract `CandidateID_Workbench.zip` to a writable folder. Open the extracted `CandidateID_Workbench` folder and double-click `Launch.exe`. The launcher starts the bundled C++ service without a console, waits for its health endpoint, and opens the frontend in the Windows default browser. Keep the small launcher status window open; clicking OK stops the service. The backend binds only to loopback and uses an available local port. No administrator access, Node.js, Python, CMake, compiler, VS Code, or network connection is needed at run time. Windows 11 x64 and its inbox system libraries are required.

The launcher assumes the zip folder has not been renamed internally and the extracted contents stay together. The archive currently uses the placeholder name `CandidateID_Workbench`; replace `CandidateID` with the assigned candidate identifier before submission if required. `Launch.exe` is the packaged launcher, not the backend itself. `backend/workbench_server.exe` and `frontend/dist/` are its runtime payload.

## Features

- Overview page with backend health/version, module links, host metadata and session operations.
- Geometry page: C++ validation of ASCII STL and temperature-point CSV; Three.js model/point-cloud display, orbit navigation, fit-to-view, temperature legend and node inspection.
- Thermal page: configurable physical parameters and 10/20/40/80 element meshes, C++ finite-element and analytical profiles, midpoint errors and CSV export.
- Performance page: fixed-seed C++ thermal batch, serial and bounded worker-pool comparison, five repetitions, medians, speedup, efficiency, host metadata and CSV export.
- Hardware page: Pico 2, Arty A7-35T and J-Link EDU Mini reference assumptions; three C++ mock adapters, telemetry, connection controls and recoverable simulated timeout. No physical hardware is connected.
- Backpropagation page: C++ forward pass, chain-rule gradients, central finite differences, simultaneous update, 100 subsequent training steps and loss chart/export.
- Asynchronous requests, explicit loading/error/empty states and cancellation that retains prior completed results.

## Architecture

```text
React 18 + Vite + Three.js (static frontend)
                 | same-origin HTTP / JSON
                 v
cpp-httplib C++17 HTTP service (backend/)
                 |
                 v
workbench_core (C++ modules: geometry, thermal, performance, hardware, ML)
```

The React code handles user interaction, chart/3D presentation and API communication. Numerical methods, file parsing/validation, workload generation and concurrency are implemented in the C++ core. The server serves the built frontend only when `WORKBENCH_WEB_ROOT` is configured by the package launcher; source-development Vite proxy behavior is retained.

## Requirements and dependency versions

### Running the extracted package

- Windows 11 x64.
- A default web browser supported by Windows ShellExecute.
- No first-run build or Internet access. The package uses static MSVC runtime linkage; its remaining imports (`KERNEL32`, `USER32`, `SHELL32`, `WINHTTP`, `WS2_32`, `ADVAPI32`) are Windows system libraries.

### Building from source

- Windows 11 x64.
- Visual Studio 18 2026 Build Tools, MSVC 19.51 or compatible, Windows 11 SDK.
- CMake 4.5.0-rc2 was used for the recorded build; the `Visual Studio 18 2026` generator is required by the supplied preset.
- Node.js 20+ and npm for the React build (recorded: Node 25.3.0, npm 11.13.0).
- Internet access for CMake FetchContent on a clean checkout unless exact-version dependencies are installed or populated locally.
- Python 3.12+ and Matplotlib 3.10.8 only to render the thermal PNG plots; the backpropagation SVG plot uses Python's standard library.
- Python 3.12 plus `requirements-report.txt` only to regenerate the PDF report or build a ZIP with a non-placeholder candidate ID.

| Dependency | Version | Purpose | License |
| --- | --- | --- | --- |
| cpp-httplib | 0.18.3 | C++ HTTP service | MIT |
| nlohmann/json | 3.12.0 | JSON | MIT |
| GoogleTest | 1.15.2 | C++ tests | BSD-3-Clause |
| React / React DOM | 18.3.1 | UI | MIT |
| Vite | 6.4.5 | Frontend build/dev server | MIT |
| Three.js | 0.187.0 | 3D visualization | MIT |
| lucide-react | 0.468.0 | UI icons | ISC |

Exact npm dependency versions are pinned in `frontend/package-lock.json`; CMake release archives have fixed versions and SHA-256 hashes in `CMakeLists.txt`. Package notices are in `THIRD_PARTY_NOTICES.txt`.

## Source build and test instructions

Run these from a VS 18 x64 Developer PowerShell at the repository root:

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset windows-msvc-x64 --parallel
ctest --test-dir build/vs18-x64 -C Release --output-on-failure
Push-Location frontend
npm ci
npm run build
Pop-Location
```

To start the source-development backend and Vite frontend, use two terminals:

```powershell
& .\build\vs18-x64\Release\workbench_server.exe
```

```powershell
Push-Location frontend
npm run dev
```

Then open the local Vite URL, usually `http://localhost:5173`. Vite proxies `/api` to `127.0.0.1:8080`. This development workflow is separate from the packaged `Launch.exe` workflow.

Create the offline Windows payload (CMake dependencies must be present or downloadable at build time):

```powershell
./scripts/package-windows.ps1 -CandidateId CandidateID
```

Before packaging, install the report-generation tools with `py -3.12 -m pip install -r requirements-report.txt`. This writes `<CandidateId>_Workbench/` and `<CandidateId>_Workbench.zip` without removing or replacing existing paths. The script stops if either output already exists. The default name is a placeholder; supply the assigned ID with `-CandidateId`. When exact npm dependencies are already installed and should not be reinstalled, add `-UseExistingNpmDependencies`; the script checks them with `npm ls` and still builds the production bundle.

### VS Code

Install Microsoft **C/C++** (`ms-vscode.cpptools`) to use the provided C++ launch configurations. Open the repository root, place the generated `<CandidateId>_Workbench` folder alongside it, select **Packaged Workbench (F5)** in Run and Debug, and press F5. That starts the same `Launch.exe`; it does not build the package first. CMake Tools is optional for interactive source configuration. The standalone launcher does not require VS Code or either extension.

## Supported files and example data

- ASCII STL only, including `solid`/`endsolid`, complete facet/loop/vertex structure and finite, non-degenerate triangles. Binary STL and native CAD solids are not supported. STL units are unspecified.
- Temperature CSV exact header: `node_id,x_m,y_m,z_m,temperature_C`; coordinates in metres and temperature in °C; unique signed 64-bit node IDs and finite values required.
- Upload limit is 16 MiB per file, verified at the exact accepted/rejected boundary in C++ tests.
- `data/examples/unit_tetrahedron_ascii.stl` is a self-created illustrative tetrahedron. `data/examples/synthetic_temperature_points.csv` is self-created synthetic data, **not an actual ANSYS export**. Provenance is recorded in `docs/geometry-and-temperature-viewers.md`.

## API

The service binds to `127.0.0.1` only. The package launcher selects an available port and sets the frontend root. Source development defaults to port 8080; set `WORKBENCH_PORT` to override it.

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/api/health` | Health check |
| GET | `/api/status` | Service name/version/status |
| POST | `/api/geometry/stl` | Parse ASCII STL request body |
| POST | `/api/geometry/temperature` | Parse exact-schema temperature CSV body |
| POST | `/api/thermal/solve` | Solve selected thermal meshes from JSON |
| GET/POST | `/api/performance/*` | Host metadata and serial/parallel batch benchmark |
| GET/POST | `/api/hardware/*` | Simulated adapter discovery and state controls |
| GET | `/api/ml/backprop` | C++ model, gradients, finite differences and training history |

Full route payloads and integration behavior are described in `docs/react-integration.md`.

## Tests and results

The baseline MSVC Release source build and CTest run reported **28/28 passed** on 2026-11-01. A separate final package build compiled the current server and launcher with static runtime; the React production build succeeded and the full browser/API walkthrough exercised every module. A fresh source verification build compiled the final server/core/launcher, but its GoogleTest post-build discovery was blocked by Windows Application Control; rerunning the existing CTest binaries passed 28/28, and numerical core source was unchanged. The thermal solver was run for all four meshes and wrote raw CSVs in `results/`; the latest fixed-seed performance CTest exported five raw repetitions for workers 1–4; the backpropagation executable exported 102 loss rows. These are local run results, not guarantees for other hardware. See `docs/FINAL_TECHNICAL_REPORT.md` and `docs/submission/REQUIREMENT_EVIDENCE.md`.

Run C++ tests with `ctest --test-dir build/vs18-x64 -C Release --output-on-failure`. Build thermal plots with `python -m pip install -r requirements.txt` then `python scripts/plot_thermal_results.py --results .\results`. `scripts/plot_backpropagation_loss.py` generates the SVG from the C++ CSV.

## Known limitations

- No binary STL, native CAD import, mesh-to-result registration, transient or multidimensional FEA, or physical hardware interface.
- Hardware devices and failure recovery are mock-only. There is no firmware deployment or FPGA programming.
- The thermal system uses the assignment's one-dimensional steady model and a dense direct solve. Performance results are workload- and machine-specific.
- Browser request cancellation stops client acceptance but cannot interrupt an already-running synchronous C++ request.
- A clean source build needs the documented compiler and dependency acquisition; only the extracted runtime package is intended to work offline and without developer tools.
- The current Vite bundle is 760 KiB minified and triggers Vite's advisory over 500 KiB; the production build succeeds.
- Candidate identifier and candidate-signed ownership declaration must be finalized before external submission.

## Submission evidence

The six-page maximum technical report is `docs/FINAL_TECHNICAL_REPORT.pdf` (with editable source at `.md`). The generated derivation guide is typed and is **not** handwritten evidence. Timestamped walkthrough captures and the API log are under `docs/demo/` when generated. The full evidence matrix and outstanding items are in `docs/submission/REQUIREMENT_EVIDENCE.md`.
