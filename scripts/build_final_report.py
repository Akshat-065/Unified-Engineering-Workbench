"""Build the concise, four-page submission report from verified run values."""
from pathlib import Path
import os

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import (BaseDocTemplate, Frame, PageTemplate, Paragraph,
                                PageBreak, Spacer, Table, TableStyle, KeepTogether)


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "docs" / "FINAL_TECHNICAL_REPORT.pdf"
CANDIDATE_ID = os.environ.get("UEW_CANDIDATE_ID", "CandidateID")
NAVY = colors.HexColor("#19324d")
TEAL = colors.HexColor("#1b7c78")
PALE = colors.HexColor("#eef3f5")


def footer(canvas, document):
    canvas.saveState()
    width, _ = letter
    canvas.setStrokeColor(colors.HexColor("#d7e0e4"))
    canvas.line(0.62 * inch, 0.48 * inch, width - 0.62 * inch, 0.48 * inch)
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#61717b"))
    canvas.drawString(0.64 * inch, 0.3 * inch, f"Unified Engineering Workbench | {CANDIDATE_ID}")
    canvas.drawRightString(width - 0.64 * inch, 0.3 * inch, f"Page {document.page}")
    canvas.restoreState()


def table(rows, widths):
    result = Table(rows, colWidths=widths, repeatRows=1, hAlign="LEFT")
    result.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), NAVY),
        ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
        ("FONTNAME", (0, 0), (-1, 0), "Helvetica-Bold"),
        ("FONTNAME", (0, 1), (-1, -1), "Helvetica"),
        ("FONTSIZE", (0, 0), (-1, -1), 8.2),
        ("LEADING", (0, 0), (-1, -1), 10.2),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#ccd6dc")),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, PALE]),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 6),
        ("RIGHTPADDING", (0, 0), (-1, -1), 6),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
    ]))
    return result


styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="ReportTitle", parent=styles["Title"], fontName="Helvetica-Bold",
                          fontSize=23, leading=27, textColor=NAVY, alignment=TA_CENTER, spaceAfter=10))
styles.add(ParagraphStyle(name="Subtitle", parent=styles["Normal"], fontSize=10, leading=14,
                          textColor=colors.HexColor("#506370"), alignment=TA_CENTER, spaceAfter=7))
styles.add(ParagraphStyle(name="Section", parent=styles["Heading1"], fontSize=15, leading=18,
                          textColor=NAVY, spaceBefore=4, spaceAfter=7, keepWithNext=True))
styles.add(ParagraphStyle(name="Subsection", parent=styles["Heading2"], fontSize=10.5, leading=13,
                          textColor=TEAL, spaceBefore=5, spaceAfter=3, keepWithNext=True))
styles.add(ParagraphStyle(name="BodyCompact", parent=styles["BodyText"], fontSize=9.2, leading=12.1,
                          spaceAfter=6))
styles.add(ParagraphStyle(name="Formula", parent=styles["BodyText"], fontName="Courier",
                          fontSize=8.6, leading=12, leftIndent=12, spaceBefore=3, spaceAfter=5,
                          backColor=PALE, borderPadding=5))
styles.add(ParagraphStyle(name="Small", parent=styles["BodyText"], fontSize=8, leading=10.2,
                          textColor=colors.HexColor("#52616b"), spaceAfter=4))


def p(text, style="BodyCompact"):
    return Paragraph(text, styles[style])


story = [
    Spacer(1, 0.22 * inch),
    p("UNIFIED ENGINEERING WORKBENCH", "ReportTitle"),
    p("PASS SCIENCE WORKS R&amp;D Internship 2026-2027 Common Assignment", "Subtitle"),
    p("Windows 11 x64 | C++17 core | React, Vite and Three.js", "Subtitle"),
    p(f"Candidate ID: {CANDIDATE_ID}" + (" (placeholder - replace before submission)" if CANDIDATE_ID == "CandidateID" else ""), "Subtitle"),
    Spacer(1, 0.12 * inch),
    p("1. Design choices and software architecture", "Section"),
    p("The workbench separates a reusable C++17 core from a cpp-httplib JSON service and a React presentation layer. Geometry parsing, finite-element computation, batch generation, parallel execution, mock device state and backpropagation remain in C++; React handles input, navigation, charts, Three.js rendering and API communication. The core modules are geometry, thermal, performance, hardware and ml, with matching GoogleTest coverage."),
    p("A Windows bootstrapper starts the backend without a console, waits for its health route, and opens the static Vite bundle served by that backend. A separate package build enables static MSVC runtime linkage. Source development uses the Visual Studio 18 2026 x64 CMake preset and Vite proxy. The VS Code F5 configuration starts the same extracted package launcher."),
    p("Dependency versions and CMake archive hashes are pinned in CMakeLists.txt; npm package versions are locked in package-lock.json. The package contains license notices. See README.md for source build, runtime, API, supported inputs and known limitations."),
    p("2. Thermal FEA formulation and verification", "Section"),
    p("The steady one-dimensional governing equation is -d/dx(kA dT/dx)=qA. Inputs are L=1 m, A=1e-4 m^2, k=10 W/(m K), q=10,000 W/m^3, T(0)=100 C, and T(L)=20 C. Material properties are constant and lateral surfaces are insulated."),
    p("For a uniform two-node linear element of length h, the local stiffness matrix and consistent generation vector are:", "BodyCompact"),
    p("K_e=(kA/h) [[1,-1],[-1,1]];   f_e=(qAh/2) [1,1]^T", "Formula"),
    p("The C++ implementation assembles each local matrix/vector into the global system. Both prescribed endpoint temperatures are enforced by eliminating fixed degrees of freedom and adjusting the free-equation right-hand side. A dense direct solve yields nodal temperatures. Maximum error is measured at element midpoints using linear FE interpolation and compared with T_exact=100-80x+500x(1-x) C."),
    table([["Elements", "Nodes", "Max midpoint error (C)"],
           ["10", "11", "1.25"], ["20", "21", "0.3125"],
           ["40", "41", "0.078125"], ["80", "81", "0.01953125"]],
          [1.25 * inch, 1.25 * inch, 3.5 * inch]),
    p("Each halving of element length reduced the interpolation error by about four, consistent with second-order convergence for linear elements on this quadratic solution. The C++ thermal executable generated the raw nodal, midpoint and convergence CSVs in results/.", "Small"),
    PageBreak(),
    p("3. CPU-parallel performance measurements", "Section"),
    p("The reproducible batch contains 40 valid thermal cases at 80 elements, with random seed 20261101. Serial execution preserves input order. Parallel execution uses the same thermal solver with a bounded std::thread pool and disjoint result slots indexed by case. A warm-up precedes five timed repetitions; medians are reported. Input/JSON work, UI rendering and comparison are outside compute timing."),
    p("Recorded host: Intel Core i5-1035G1, 4 physical and 8 logical cores, 12,611,039,232 bytes RAM, MSVC 19.51, Release flags /O2 /Ob2 /DNDEBUG. The raw five-repetition records and metadata for every worker count are in results/performance/task_d_workers_1.csv through task_d_workers_4.csv."),
    table([["Workers", "Serial median (ms)", "Parallel median (ms)", "Speedup", "Efficiency"],
           ["1", "7.0947", "7.1022", "0.9989x", "99.89%"],
           ["2", "6.6252", "3.9645", "1.6711x", "83.56%"],
           ["3", "5.8538", "2.6059", "2.2464x", "74.88%"],
           ["4", "6.7014", "2.1907", "3.0590x", "76.48%"]],
          [0.72 * inch, 1.33 * inch, 1.38 * inch, 1.0 * inch, 1.05 * inch]),
    p("For all worker counts, the recorded maximum absolute temperature difference was 0 C, within the 1e-8 C criterion. The one-worker efficiency slightly above 100% results from variation between two short-run medians and is not a superlinear scaling claim. These measurements characterize one workload on one host only; scheduling, frequency, heat and system load affect timing."),
    p("4. Embedded design and backpropagation", "Section"),
    p("The reference embedded design documents a Raspberry Pi Pico 2/RP2350, Digilent Arty A7-35T Rev E.2, and SEGGER J-Link EDU Mini V1. docs/hardware-integration.md includes the labelled host/board/adapter/power/ground block diagram and connector, power, ground, voltage, protocol, level-shifting and signal-ownership table. Three C++ mock adapters implement discovery, capabilities, connection controls, telemetry and recoverable simulated timeout. No physical board or debug signal is controlled."),
    p("Task F uses x=1, target=2, eta=0.1, initial (w,b,v,c)=(0.5,0,1,0). The hidden activation/output are 0.4621171572600097 and the initial loss is 1.1825418189970167. Initial gradients are dw=db=-1.2094644752400612, dv=-0.7106820474859469, dc=-1.5378828427399902. A single simultaneous update gives (w,b,v,c)=(0.620946447524, 0.120946447524, 1.071068204749, 0.153788284274) and loss 0.6857737650448423."),
    p("Central differences use epsilon=1e-6. Absolute gradient errors are about 1.13e-10 (w), 1.13e-10 (b), 5.25e-11 (v), and 1.12e-10 (c), each below 1e-5. After 100 additional updates, loss was 1.08049848736e-20; the C++ CSV has 102 states. The typed derivation guide is not handwritten submission evidence."),
    PageBreak(),
    p("5. Testing, packaging and limitations", "Section"),
    p("Verification was performed on 2026-11-01 with CMake 4.5.0-rc2, Visual Studio 18 2026 x64, MSVC 19.51.36260, Node 25.3.0 and npm 11.13.0. The baseline Release source build succeeded; CTest passed 28/28 tests for core/status, thermal, geometry, performance, hardware mocks and backpropagation. A fresh source compile after adding the launcher/static server compiled the server, core, command-line tools and launcher, but GoogleTest discovery was blocked when Windows Application Control denied running its new test binary. An in-place relink hit a lock held by two pre-existing server processes, which were left running. No numerical core source changed after the passing CTest run. The Vite 6.4.5 production build passed after a browser test found and fixed a missing Square icon import. Its 760.46 kB minified JS chunk triggered the 500 kB advisory."),
    p("The separate package build generated Launch.exe and workbench_server.exe with static MSVC runtime linkage. dumpbin /dependents listed only Windows system imports. The packaged service smoke walkthrough returned GET /api/health status=ok, served the static frontend, parsed both sample files, solved all four meshes, ran a five-repeat CPU comparison, returned three simulated devices and returned 102 C++ model-history points."),
    p("Limitations: only ASCII STL is supported; native CAD and binary STL are excluded. The temperature CSV example is synthetic and is not an ANSYS export. Uploads are capped at 16 MiB. Thermal analysis is steady, one-dimensional and constant-property. The benchmark uses a dense solver and measured speedups are hardware-specific. Hardware devices and timeout recovery are simulated only. Browser cancellation cannot preempt an in-flight synchronous server request. The candidate must replace CandidateID, sign the ownership affirmation, prepare the handwritten appendix, and perform an ordinary browser-opening double-click check on a clean Windows 11 machine. The included launcher self-test verifies backend startup and static root only. The full requirement matrix is in docs/submission/REQUIREMENT_EVIDENCE.md."),
]


class Report(BaseDocTemplate):
    def __init__(self, path):
        super().__init__(str(path), pagesize=letter,
                         leftMargin=0.68 * inch, rightMargin=0.68 * inch,
                         topMargin=0.62 * inch, bottomMargin=0.68 * inch,
                         title="Unified Engineering Workbench - Final Technical Report",
                         author="Unified Engineering Workbench")
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height,
                      leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates([PageTemplate(id="report", frames=frame, onPage=footer)])


if __name__ == "__main__":
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    Report(OUTPUT).build(story)
    print(OUTPUT)
