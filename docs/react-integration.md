# React and C++ module integration

The React application calls the local C++ HTTP service through Vite's `/api` proxy. React owns input controls, loading/error/empty states, navigation, SVG/Three.js display, and CSV downloads. Geometry parsing, thermal solving, benchmark generation and execution, hardware adapter state, and all backpropagation calculations remain in C++.

| React page | C++ API | Interaction |
| --- | --- | --- |
| Overview | `/api/health`, `/api/status`, `/api/performance/hardware` | Backend status, host CPU/RAM, module navigation, in-session recent operation list |
| Geometry Viewer | `POST /api/geometry/stl`, `POST /api/geometry/temperature` | ASCII STL and temperature CSV parsing; Three.js OrbitControls rotates, pans and zooms; fit-to-view and point inspection |
| Thermal Analysis | `POST /api/thermal/solve` | Validated model inputs and selected 10/20/40/80 element meshes; FEA and analytical temperature profiles; midpoint error comparison; CSV export |
| Performance | `GET /api/performance/hardware`, `POST /api/performance/benchmark` | Serial and worker-pool benchmark configuration, repetition chart, speedup/efficiency, measured CSV export |
| Hardware | `/api/hardware/devices` and per-device connect/disconnect routes | Discovery, connection state, telemetry and injected simulated timeout/recovery; always labelled simulated |
| Backpropagation | `GET /api/ml/backprop` | C++ forward pass, gradients, central difference checks, one update, 100 subsequent updates, loss curve and CSV download |

## Async requests and retained results

Long-running uploads, thermal solves, performance benchmarks, and model requests use `fetch` with `AbortController`. The UI remains interactive while a request is pending. A new result replaces the displayed result only after a successful response. If a request is cancelled or fails, the previous completed result remains visible. A cancelled browser request does not currently stop an already-running synchronous C++ server handler; it only stops the client from accepting that response. A later request may be served concurrently by cpp-httplib's worker pool.

Thermal and performance progress is indeterminate because the current APIs return one response after completing the calculation. This is reported explicitly as a running status rather than an invented percentage. Geometry and backpropagation requests show loading and cancellation controls as well.

## Thermal endpoint schema

`POST /api/thermal/solve` expects JSON with a `parameters` object containing `length_m`, `area_m2`, `conductivity_w_per_m_k`, `heat_generation_w_per_m3`, `left_temperature_c`, `right_temperature_c`, plus `meshes`, a non-empty selection from `[10, 20, 40, 80]`. It returns each selected mesh's C++ nodal and midpoint FEA values, analytical values, and maximum absolute midpoint error. Invalid input returns HTTP 400 with an actionable `error` string.

The browser exports only values returned by the C++ API; it does not calculate a second solution. Thermal CSV rows distinguish nodes from midpoints. Performance and backpropagation downloads likewise serialize returned C++ results.
