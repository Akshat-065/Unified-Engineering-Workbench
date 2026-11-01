# Task D: CPU parallel thermal batch benchmark

## Workload and solver

The benchmark calls `workbench::thermal::solve` for every case in both modes. No separate or approximate numerical method is used. Each generated case retains the base thermal geometry and boundary temperatures, and varies conductivity uniformly over [5, 25] W/(m·K) and volumetric generation over [5,000, 15,000] W/m³ using `std::mt19937_64` and fixed default seed `20261101`. The API and CLI use 40 independent cases and 80 elements by default. The seed is configurable from the CLI/API for reproducibility tests.

## Worker-pool design

The serial path traverses the input vector from beginning to end. The parallel path starts a bounded set of `std::thread` workers. Workers reserve case indexes with an atomic fetch-add, solve that case using the same thermal solver, and write only to the result slot at that index. This preserves deterministic input ordering regardless of which worker finishes first. An atomic stop flag and mutex-protected `exception_ptr` propagate worker failures after all created workers are joined. Worker count is limited to the requested count, number of cases, and `std::thread::hardware_concurrency()` (falling back to one if unavailable).

## Measurement and correctness

There is one warm-up pass in each mode. The benchmark then measures five repetitions with `steady_clock`; elapsed intervals include solver calls and result-vector allocation, but exclude HTTP/file input parsing, JSON serialization, comparison, plotting, compilation, and UI rendering. Serial and parallel result vectors are compared after each timed interval, in node and element-midpoint temperatures. Any difference above 1e-8 °C fails the benchmark. The reported time is the median of the five raw measurements. Speedup is `median_serial_ms / median_parallel_ms`; efficiency is speedup divided by the effective parallel worker count. A one-worker parallel run is valid and may show overhead rather than a speedup.

The UI separately measures browser launch to the first successful health/status response and sends that duration as `launch_to_ready_ms`. This startup/readiness measurement is not included in compute timing. CLI runs record process startup through argument/configuration readiness in the same field; it is not a measurement of UI or HTTP server readiness. A blank CSV field indicates that the C++ benchmark library ran without a frontend/CLI readiness sample.

## Hardware and limitations

CPU model, physical/logical core counts and installed RAM are collected from Windows system APIs/registry (or platform-specific system files on Linux); compiler and optimization information is compiled into the C++ result. Physical core count can fall back to logical count if the platform cannot expose topology. CPU affinity, virtual-machine limits, thermal throttling, background system load, and power settings can affect actual timing. Do not compare results across unlike hardware/build configurations as if they were controlled experiments.

The existing thermal solver uses dense Gaussian elimination. To bound memory and compute, Task D limits mesh size to 512 and rejects a workload when `case_count * mesh_size^3 > 250,000,000`. The worker pool can be slower than serial for small workloads because thread creation and scheduling add overhead. API benchmarks are synchronous requests; while the UI shows an indeterminate progress indicator, the actual parallel work occurs in the C++ worker pool.

## Build, run, and raw CSV

From the repository root in a VS 18 Developer PowerShell:

```powershell
cmake --preset windows-msvc-x64
cmake --build build/vs18-x64 --config Release --target workbench_performance workbench_performance_tests
ctest --test-dir build/vs18-x64 -C Release -R PerformanceBatch --output-on-failure
.\build\vs18-x64\Release\workbench_performance.exe --cases 40 --mesh 80 --workers 4 --seed 20261101 --output results\performance\task_d_raw.csv
```

The CLI creates its relative output directory if needed. The `PerformanceBatch.ExportRawWorkerComparisons` test also runs this 40-case workload for worker counts 1–4 and writes `results/performance/task_d_workers_1.csv` through `task_d_workers_4.csv`. The CSV contains five raw serial and parallel wall times plus correctness delta, medians, speedup, efficiency, workload, seed, worker count and detected hardware/build fields. Do not report a speedup without inspecting those measured outputs.

## Recorded run in this workspace

The final Release CTest run generated the CSVs on an Intel Core i5-1035G1 host (4 physical / 8 logical cores, 12,611,039,232 bytes RAM), MSVC 19.51, CMake Release defaults `/O2 /Ob2 /DNDEBUG`. Workload was 40 cases × 80 elements, seed `20261101`; all reported serial/parallel maximum temperature differences were 0 °C. Observed medians:

| Workers | Serial median (ms) | Parallel median (ms) | Observed speedup | Efficiency |
|---:|---:|---:|---:|---:|
| 1 | 7.0947 | 7.1022 | 0.9989× | 99.89% |
| 2 | 6.6252 | 3.9645 | 1.6711× | 83.56% |
| 3 | 5.8538 | 2.6059 | 2.2464× | 74.88% |
| 4 | 6.7014 | 2.1907 | 3.0590× | 76.48% |

This is one short local measurement, not a general performance guarantee. Notice the serial medians varied between runs, so background load and CPU frequency materially affect this small workload. The CTest-generated CSV launch-to-ready fields are blank because the C++ benchmark test does not start the React/API application; the dashboard measures and displays that value during a real application session.
