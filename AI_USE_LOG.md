# AI use log

This log records the AI-assisted work visible in the project conversation and repository audit. It is not a substitute for the candidate's own contribution record or review.

## Tool and model

- Tool: OpenAI Codex coding agent, GPT-6 (the assistant identity exposed in this work session).
- Session date for final packaging audit: 2026-11-01.
- AI output was used to draft and edit C++ modules, React components, CMake configuration, tests, documentation, and this submission preparation. The candidate remains responsible for understanding and validating the submitted work.

## Representative user prompts and outputs

| User request (short description) | Relevant AI-assisted output |
| --- | --- |
| Implement the 1D steady thermal FEA for L=1 m, A=1e-4 m², k=10 W/(m K), q=10000 W/m³, and fixed end temperatures, with 10/20/40/80 meshes and midpoint error | `src/thermal/solver.cpp`, `include/thermal/solver.h`, `tests/thermal/solver_test.cpp`, thermal result generation and `docs/thermal-analysis.md` |
| Implement Task B ASCII STL and exact-schema ANSYS temperature CSV parsing in C++ with validation and Three.js view | `src/geometry/*`, `include/geometry/*`, geometry tests, `frontend/src/GeometryWorkspace.jsx`, `frontend/src/ThreeViewport.jsx`, example datasets, and geometry docs |
| Implement fixed-seed serial/parallel batch benchmarking with bounded workers, five repetitions, median/speedup/efficiency and correctness delta | `src/performance/*`, `include/performance/*`, tests, raw CSV outputs, performance dashboard and `docs/task-d-performance.md` |
| Implement Task E hardware adapter design and simulated microcontroller, FPGA and JTAG adapters | `src/hardware/*`, `include/hardware/*`, tests, hardware dashboard and interface/safety documentation |
| Implement Task F exact backpropagation, simultaneous update, central differences and training | `src/ml/*`, `include/ml/*`, tests, generated loss CSV/SVG, dashboard and derivation guide |
| Integrate all completed C++ modules into React with async requests, cancellation and exports | `src/api/server.cpp`, React workspaces, integration documentation and dashboard |
| Prepare the project for submission, Windows packaging, final report, evidence and audit | Windows package launcher/hosting configuration, packaging script, README, report, ownership and evidence documents |

These are concise descriptions of prompts actually supplied in the project conversation, not invented additional exchanges. The exact prompts are retained in the conversation history.

## Relevant generated content and candidate work

The AI-assisted source changes are identified above and in Git history. Git records the implementation commits under author name `Akshat`; Git author metadata is not proof of legal authorship or of which lines were independently edited. No separate candidate-authored change log was supplied in the materials available for this audit, so this log does not claim specific independent candidate modifications. The candidate should add a short dated record of their own review, edits, and handwritten work before submitting.

## Independent verification performed

On 2026-11-01, the Windows x64 source build completed with MSVC 19.51.36260 and CMake 4.5.0-rc2; the Vite production build completed; CTest ran 28 tests and reported 28/28 passed. The C++ thermal executable generated all four requested meshes; the performance test exported five repetitions per worker count; the backpropagation executable wrote 102 loss rows and reported finite-difference errors below 1e-5. The package-specific build and its runtime/dependency checks are documented in the final audit checklist. These checks independently exercise generated algorithms but do not establish that the candidate authored or understands them.

## Unsuitable AI-generated result identified and checked

An earlier AI-authored performance summary in `docs/task-d-performance.md` no longer agreed with refreshed raw worker CSVs. At the first final-audit snapshot, I parsed five records per worker file and found that the three-worker median (7.4723 ms serial / 3.1605 ms parallel) did not match the prose table. This exposed stale measurement prose as unsuitable evidence. A later final CTest run regenerated the raw files; the current docs/report were updated again to that run's current CSV values (5.8538 ms / 2.6059 ms for three workers). The raw CSVs, not a remembered table, are the source of truth.

The browser walkthrough also found an incorrect AI-generated UI implementation detail: `GeometryWorkspace.jsx` rendered a `Square` icon in its loading/cancel row without importing it. Selecting an STL caused a `ReferenceError` and blanked the React view. The missing import was added, the Vite production bundle rebuilt, and the final captured STL upload screenshot showed the four-triangle model in Three.js. The failure and corrected behavior were observed in the actual browser console and UI, not inferred from code inspection.

## Disclosure boundary

Generated mathematical explanations and plots are typed/digital; neither is represented as handwritten evidence. No physical board connection, firmware upload, FPGA programming, or real hardware result was generated. Candidate ID and candidate signature/ownership affirmation remain to be supplied by the candidate.
