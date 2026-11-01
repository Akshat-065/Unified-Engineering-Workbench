# Assignment requirement-to-evidence audit

Audit performed 2026-11-01 against the assignment requirements captured in the project conversation. “Verified” means an executable check or artifact was inspected during this audit; documentation presence alone is marked documented, not independently verified.

| Requirement | Evidence | State / limitation |
| --- | --- | --- |
| C++17 core separate from React/API | `include/`, `src/`, `CMakeLists.txt`; 28-test CTest run | Verified build/tests |
| Windows 11 x64 source configuration | `CMakePresets.json`; VS 18 2026, x64 CMake cache | Verified package and source build configuration; clean-machine configure needs pinned sources or network |
| One-click extracted Windows app, no dev runtime/network/admin | `Launch.exe`, `backend/workbench_server.exe`, `frontend/dist/`, `scripts/windows_launcher.cpp`, `scripts/smoke_package_launch.ps1` | Static MSVC runtime; package self-test starts its bundled backend and confirms health and static root. A separate clean Windows 11 device/browser test remains advisable. |
| HTTP health/status and module API | `src/api/server.cpp`; `docs/demo/package-smoke.log` | Health, root, status, geometry, thermal, performance, simulated hardware and ML routes exercised against the packaged C++ service. |
| React pages and C++ integration | `frontend/src/`, `docs/react-integration.md`, timestamped UI screenshots | Production build passed; screenshot walkthrough exercised all requested pages and controls. |
| ASCII STL parsing/visualization | C++ parser/tests, example STL, `docs/demo/screenshots/02_ASCII_STL.png` | Four-facet ASCII model parsed and rendered. Binary STL/native CAD excluded. |
| Exact temperature CSV parsing/point cloud | C++ parser/tests, synthetic CSV with provenance, Geometry page | Tests passed; CSV is explicitly synthetic, not ANSYS. |
| Thermal FEA exact parameters/equations | `src/thermal/solver.cpp`, `docs/thermal-analysis.md`, raw CSV | Four meshes executed; max midpoint errors 1.25, 0.3125, 0.078125, 0.01953125 °C. Tests verify boundary conditions, analytical solution and second-order mesh convergence. |
| Serial/parallel agreement <= 1e-8 °C | `results/performance/task_d_workers_{1,2,3,4}.csv`, tests | All four refreshed worker files report maximum difference 0 °C for 40 x 80, seed 20261101; CTest correctness test passed. |
| Fixed-seed, five-repeat benchmark and metadata | Worker CSVs and `docs/task-d-performance.md` | Five raw records for each worker count; values are machine-specific and reported as measurements, not a speedup promise. |
| Three embedded device types/interface design | `docs/hardware-integration.md`, hardware C++/UI/tests | Documented and tested as mock adapters only; no physical devices connected. |
| Manual backprop and finite difference <= 1e-5 | `src/ml/`, tests, `results/backpropagation_loss.csv`, docs | Tests passed; generated numerical errors are ~5e-11 to ~1.2e-10, below threshold. Typed guide is not handwritten evidence. |
| C++ tests | `ctest --test-dir build/vs18-x64 -C Release --output-on-failure` | Baseline run passed 28/28 on 2026-11-01. A fresh post-bootstrap test-binary discovery was blocked by Windows Application Control; module sources were unchanged after the passing run. |
| Raw CSV and labelled plots | `results/*.csv`, thermal PNGs, loss SVG | C++ result CSVs and labelled thermal/ML charts are present. |
| <=6 page final report | `docs/FINAL_TECHNICAL_REPORT.pdf`, editable Markdown and builder | Rendered and visually inspected at 3 pages. |
| Demo video or timestamped screenshots with logs | `docs/demo/WALKTHROUGH.md`, `docs/demo/package-smoke.log`, `docs/demo/screenshots/` | Nine timestamped real browser captures cover all requested modules and timeout/recovery. |
| Git repository and history | `git log`; remote `origin` | Existing stage-wise commits are present and were pushed; no history is fabricated by this audit. |
| AI disclosure / ownership | `AI_USE_LOG.md`, `OWNERSHIP.md` | AI-assisted work and a stale AI-generated benchmark summary discrepancy are disclosed. Candidate must complete and sign ownership affirmation. |
| Candidate ID | package/ZIP naming | `CandidateID` remains a placeholder until the assigned ID is provided. |

## Final blockers before external submission

1. Replace `CandidateID` with the assigned identifier using `scripts/package-windows.ps1 -CandidateId <id>`.
2. Candidate completes and signs the ownership affirmation and adds their own dated review/modification note to `AI_USE_LOG.md`.
3. Candidate performs an ordinary double-click launch of the extracted ZIP on a clean Windows 11 x64 device; the included self-test verifies backend/static startup but skips opening the desktop browser.
4. Candidate prepares the handwritten appendix separately; the typed guide is not that deliverable.

## Checks executed for this audit

- Baseline `cmake --build build/vs18-x64 --config Release --parallel` — succeeded. After launcher/static-serving changes, a fresh source build compiled the modified server, core and launcher, but GoogleTest discovery could not execute the newly built test binary because Windows Application Control blocked it. An in-place rebuild also encountered a file lock held by two pre-existing server processes; those processes were left untouched.
- `npm run build` in `frontend/` — succeeded; Vite emitted the 500 KiB advisory for the 760 KiB minified JS bundle.
- `ctest --test-dir build/vs18-x64 -C Release --output-on-failure` — 28/28 passed.
- `workbench_thermal.exe .\results` — generated 10/20/40/80 element results and convergence CSV.
- `workbench_ml.exe .\results\backpropagation_loss.csv` — generated 102 rows and reported central-difference checks under 1.2e-10 absolute error.
- Packaged backend smoke check — `/api/health` returned `ok`, `/` served the Vite index with HTTP 200, `/api/status` returned version 0.1.0, geometry returned four facets/eight points, thermal returned all four meshes, performance returned five measurements with zero temperature delta, hardware discovery returned three simulated devices, and ML returned 102 history rows. See `docs/demo/package-smoke.log` and `docs/demo/WALKTHROUGH.md`.
- `dumpbin /dependents` — packaged server and launcher import only Windows system DLLs; no MSVC runtime DLL dependency was listed.
