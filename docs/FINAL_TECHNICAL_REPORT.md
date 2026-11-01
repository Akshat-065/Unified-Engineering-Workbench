# Unified Engineering Workbench — Final Technical Report

**Assignment:** PASS SCIENCE WORKS R&D Internship 2026–2027 common assignment
**Target:** Windows 11 x64; C++17 core; React/Vite frontend
**Evidence date:** 2026-11-01
**Candidate:** CandidateID (placeholder; replace before submission)

## 1. Design choices and software architecture

The workbench uses a C++17 core library (`workbench_core`) for numeric computation, concurrency, validation and file parsing. A separate cpp-httplib server adapts core results to JSON. React 18 and Vite provide the user interface; Three.js handles 3D geometry and temperature point display. The frontend does not implement numerical solvers. The Windows package adds a small Win32 launcher, a static-runtime C++ backend executable and the Vite production bundle. The backend binds to loopback and serves the extracted static bundle when launched by the package bootstrapper.

The core is divided into `geometry`, `thermal`, `performance`, `hardware`, and `ml`. GoogleTest suites mirror these modules. Exact dependency versions and SHA-256 release-archive hashes are recorded in `CMakeLists.txt`; npm direct and transitive versions are locked in `frontend/package-lock.json`. `.vscode/launch.json` contains a one-action F5 entry for the same packaged launcher. Source compilation remains separately available using CMake.

The UI exposes a dashboard, ASCII STL and temperature CSV viewer, thermal analysis, fixed-seed performance comparison, simulated hardware adapters, and backpropagation. Requests use asynchronous fetch. Failed or cancelled requests do not overwrite prior completed UI results. A browser cancellation cannot preempt a C++ request already executing synchronously.

## 2. Thermal finite element formulation and verification

The solved one-dimensional steady equation is

\[
-\frac{d}{dx}\left(kA\frac{dT}{dx}\right)=qA,\quad T(0)=100^\circ C,\quad T(1)=20^\circ C.
\]

Inputs are `L=1 m`, `A=10^-4 m²`, `k=10 W/(m K)`, `q=10,000 W/m³`; properties are constant and lateral surfaces insulated. For a uniform two-node linear element of length `h`, the local matrix and consistent source vector are

\[
K_e=\frac{kA}{h}\begin{bmatrix}1&-1\\-1&1\end{bmatrix},\qquad
f_e=\frac{qAh}{2}\begin{bmatrix}1\\1\end{bmatrix}.
\]

Element contributions are assembled into the global system. Both prescribed temperatures are imposed by eliminating fixed degrees of freedom and adjusting the free-equation right-hand side. The C++ dense direct solve returns nodal values; the reported maximum error is evaluated at element midpoints using the actual linear FE interpolation and compared with `T_exact=100-80x+500x(1-x)` °C.

| Elements | Nodes | Maximum absolute midpoint error (°C) |
|---:|---:|---:|
| 10 | 11 | 1.25 |
| 20 | 21 | 0.3125 |
| 40 | 41 | 0.078125 |
| 80 | 81 | 0.01953125 |

Each halving of element length reduces this interpolation error by approximately four, consistent with second-order convergence for this quadratic exact solution approximated by linear elements. CTest covers the prescribed endpoints, invalid zero/negative inputs, node counts, the analytical profile, midpoint interpolation and refinement trend. The solver executable wrote `results/thermal_results.csv` and `results/thermal_convergence.csv` during this audit.

## 3. CPU-parallel performance measurements

The batch contains 40 independent valid thermal cases, 80 elements each, generated with seed `20261101`. Serial execution preserves input order. Parallel runs use the same thermal solver and bounded `std::thread` workers, with disjoint result slots indexed by input case; the worker pool joins before returning. A warm-up precedes five timed repetitions. Timings exclude JSON/file parsing and UI work; reports use the median. All raw observations, detected machine/compiler data and correctness deltas are in `results/performance/task_d_workers_*.csv`.

Recorded host: Intel Core i5-1035G1, 4 physical / 8 logical cores, 12,611,039,232 bytes RAM, MSVC 19.51, Release defaults `/O2 /Ob2 /DNDEBUG`. Each worker file reports a maximum serial/parallel temperature difference of 0 °C, below the `1e-8 °C` requirement.

| Workers | Serial median (ms) | Parallel median (ms) | Speedup | Efficiency |
|---:|---:|---:|---:|---:|
| 1 | 7.0947 | 7.1022 | 0.9989× | 99.89% |
| 2 | 6.6252 | 3.9645 | 1.6711× | 83.56% |
| 3 | 5.8538 | 2.6059 | 2.2464× | 74.88% |
| 4 | 6.7014 | 2.1907 | 3.0590× | 76.48% |

This is one short run on one host, not a general speedup guarantee. The >100% one-worker efficiency reflects run-to-run timing variation between serial and parallel medians; it is not evidence of superlinear scaling. Thread creation, OS scheduling, CPU frequency, background activity and the small dense workload affect measurements. The CSVs retain raw repetitions so the medians can be checked.

## 4. Embedded design and backpropagation verification

The documented reference design assumes a Raspberry Pi Pico 2/RP2350 microcontroller, Digilent Arty A7-35T Rev E.2 FPGA board, and SEGGER J-Link EDU Mini V1 debug adapter. The block diagram and connector, power, ground, voltage-domain, protocol, level-shifting and pin-ownership table are in `docs/hardware-integration.md`. Three C++ mock adapters cover discovery, capability reporting, connect/disconnect, telemetry and a recoverable simulated timeout. UI and documents explicitly state no physical device, firmware deployment or FPGA programming is implemented.

For Task F, the C++ model uses `x=1`, `y=2`, `eta=0.1`, and `(w,b,v,c)=(0.5,0,1,0)`. The initial hidden activation and output are both `tanh(0.5)=0.4621171572600097`; initial loss is `1.1825418189970167`. Gradients are `dw=db=-1.2094644752400612`, `dv=-0.7106820474859469`, `dc=-1.5378828427399902`. A simultaneous first update produces `(w,b,v,c)=(0.6209464475240061,0.1209464475240061,1.0710682047485947,0.1537882842739990)` and loss `0.6857737650448423`.

Central finite differences use epsilon `10^-6`. The exported errors are approximately `1.13e-10` for `w`, `1.13e-10` for `b`, `5.25e-11` for `v`, and `1.12e-10` for `c`; all are below `1e-5`. After 100 additional simultaneous steps, loss is `1.0804984873618949e-20`. `results/backpropagation_loss.csv` contains the initial state and 101 updated states (102 rows). The generated derivation guide is typed; the candidate must provide the separate handwritten appendix.

## 5. Testing, packaging and limitations

On 2026-11-01, the baseline CMake Release source build succeeded with the Visual Studio 18 2026 x64 generator and MSVC 19.51.36260; its CTest run passed 28/28 core/status, thermal, geometry, performance, mock hardware and backpropagation tests. After adding the Windows launcher and static frontend serving, a separate source verification build compiled the updated core, server, command-line tools and launcher, but GoogleTest's post-build discovery was blocked because Windows Application Control denied running its new test binary. An in-place rebuild also hit a lock held by two pre-existing server processes; they were left running. No numerical core source changed after the passing CTest run. The separate package build uses static MSVC runtime linkage; `dumpbin /dependents` listed only Windows system DLL imports. The Vite 6.4.5 production build succeeded after a browser walkthrough exposed and fixed a missing `Square` icon import. Vite reports a 760.46 kB minified JavaScript chunk above its 500 kB advisory threshold. The final API/browser smoke walkthrough served the built frontend and exercised geometry parsing, all four meshes, five-repeat performance, three simulated devices and the C++ model history.

The app supports ASCII STL, not binary STL/native CAD; the CSV example is synthetic, not ANSYS output. Geometry uploads are capped at 16 MiB, with exact boundary tests. Thermal analysis is steady, 1D and uses constant properties. The benchmark solver is dense and workload-limited. Hardware interactions are mock-only. The browser is an OS-provided default-browser dependency of the package launcher. The launcher self-test checks the extracted-folder backend and static root; the candidate still needs to perform an ordinary browser-opening double-click on a clean Windows 11 machine. Candidate ID and signed ownership declaration remain candidate-supplied. See `docs/submission/REQUIREMENT_EVIDENCE.md` for requirement-by-requirement evidence and remaining gates.
