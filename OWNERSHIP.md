# Ownership and third-party attribution

## Original project implementation

Git history records the project implementation commits under `Akshat <akshatnewatia1@gmail.com>`, beginning with `0b1ab8e` and continuing through the current preparation changes. This statement reports repository metadata only. It does not independently establish copyright ownership, employment/assignment rights, originality, or the identity of each contributor. The candidate must verify and sign the ownership affirmation below before submission.

**Candidate ownership affirmation (complete and sign):**

> I, ____________________, confirm that I have reviewed the submitted source and have the right to submit it for this assignment. I have identified reused material and third-party components below, and have not represented generated evidence or simulated hardware as independently performed physical work.
>
> Candidate ID: ____________________   Date: ____________________   Signature: ____________________

## Project-authored work and AI assistance

The C++ core, React UI, tests, example data, documentation, numerical result artifacts, and Windows packaging source are project implementation material recorded in this repository. AI assistance is disclosed in `AI_USE_LOG.md`. The log distinguishes recorded prompts and outputs from candidate-authored changes; the candidate must add their own review/change record and verify the legal ownership assertion above. The assignment-specific result data must be reproducible from the documented C++ executables and commands.

## Reused code and third-party libraries

The project does not intentionally copy third-party application source into its own `src/` or `frontend/src/` files. It uses these dependencies:

| Component | Version | Use | License / notice location |
| --- | --- | --- | --- |
| cpp-httplib | 0.18.3 | Header-only HTTP server | MIT; bundled notice `THIRD_PARTY_NOTICES.txt` |
| nlohmann/json | 3.12.0 | Header-only JSON library | MIT; bundled notice `THIRD_PARTY_NOTICES.txt` |
| GoogleTest | 1.15.2 | Build-time tests only; not in runtime package | BSD-3-Clause; bundled notice `THIRD_PARTY_NOTICES.txt` |
| React and React DOM | 18.3.1 | Frontend | MIT; frontend package notice |
| Three.js | 0.187.0 | 3D viewer | MIT; frontend package notice |
| Vite | 6.4.5 | Frontend build and dev server | MIT; frontend package notice |
| @vitejs/plugin-react | 4.3.4 | Frontend build | MIT; frontend package notice |
| lucide-react | 0.468.0 | Icons | ISC; frontend package notice |

Exact direct/transitive npm packages are pinned in `frontend/package-lock.json`. The complete generated license/notice inventory for bundled frontend dependencies is in `frontend/THIRD_PARTY_NOTICES.txt`. Microsoft system libraries imported by the Windows binaries are part of the Windows 11 operating system; the package uses static MSVC runtime linkage and includes no Visual C++ redistributable DLLs. CMake build dependencies are fetched at pinned versions and hashes documented in `CMakeLists.txt`.

## Example datasets and provenance

`data/examples/unit_tetrahedron_ascii.stl` is a self-created illustrative dataset with unspecified STL units. `data/examples/synthetic_temperature_points.csv` is self-created synthetic data in metres and °C and is not an actual ANSYS export. Provenance is documented in `docs/geometry-and-temperature-viewers.md`.
