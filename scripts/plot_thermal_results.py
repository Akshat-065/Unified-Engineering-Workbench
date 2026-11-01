"""Render plots from CSV files produced by the C++ thermal solver."""

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt


def read_rows(path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results", type=Path, default=Path("results"), help="directory with solver CSV files")
    args = parser.parse_args()
    rows = read_rows(args.results / "thermal_results.csv")
    convergence = read_rows(args.results / "thermal_convergence.csv")
    if not rows or not convergence:
        raise SystemExit("Thermal CSV files are empty; run workbench_thermal first.")

    meshes = sorted({int(row["mesh_elements"]) for row in rows})
    colors = {10: "#3977a8", 20: "#d27642", 40: "#5d8c55", 80: "#9868a5"}
    analytical_rows = [row for row in rows if row["point_type"] == "node" and int(row["mesh_elements"]) == meshes[-1]]
    analytical_rows.sort(key=lambda row: float(row["x_m"]))

    figure, axis = plt.subplots(figsize=(9, 5.4), constrained_layout=True)
    axis.plot([float(row["x_m"]) for row in analytical_rows],
              [float(row["analytical_temperature_C"]) for row in analytical_rows],
              color="#202a31", linewidth=2.2, label="Analytical solution")
    for mesh in meshes:
        mesh_rows = [row for row in rows if int(row["mesh_elements"]) == mesh]
        mesh_rows.sort(key=lambda row: float(row["x_m"]))
        axis.plot([float(row["x_m"]) for row in mesh_rows],
                  [float(row["finite_element_temperature_C"]) for row in mesh_rows],
                  linewidth=1.0, color=colors.get(mesh), label=f"Piecewise-linear FEM, N={mesh}")
    axis.set(title="Steady-state bar temperature — all meshes", xlabel="Position, x (m)", ylabel="Temperature (°C)")
    axis.grid(True, alpha=0.25)
    axis.legend(title="Solution / mesh identification")
    figure.savefig(args.results / "thermal_temperature_all_meshes.png", dpi=180)
    plt.close(figure)

    for mesh in meshes:
        mesh_rows = [row for row in rows if int(row["mesh_elements"]) == mesh]
        mesh_rows.sort(key=lambda row: float(row["x_m"]))
        figure, axis = plt.subplots(figsize=(9, 5.4), constrained_layout=True)
        axis.plot([float(row["x_m"]) for row in mesh_rows],
                  [float(row["analytical_temperature_C"]) for row in mesh_rows],
                  color="#202a31", linewidth=2.0, label="Analytical solution")
        axis.plot([float(row["x_m"]) for row in mesh_rows],
                  [float(row["finite_element_temperature_C"]) for row in mesh_rows],
                  marker="o", markersize=3, linewidth=1.0, color=colors.get(mesh), label=f"Linear FEM, N={mesh}")
        axis.set(title=f"Analytical and finite element temperatures — N={mesh}", xlabel="Position, x (m)", ylabel="Temperature (°C)")
        axis.grid(True, alpha=0.25)
        axis.legend(title=f"Mesh identification: N={mesh} elements")
        figure.savefig(args.results / f"thermal_comparison_N{mesh}.png", dpi=180)
        plt.close(figure)

    convergence.sort(key=lambda row: int(row["mesh_elements"]))
    figure, axis = plt.subplots(figsize=(7.2, 5.2), constrained_layout=True)
    axis.loglog([int(row["mesh_elements"]) for row in convergence],
                [float(row["max_absolute_midpoint_error_C"]) for row in convergence],
                marker="o", linewidth=1.7, color="#3977a8", label="Maximum midpoint error")
    axis.set(title="Thermal mesh convergence", xlabel="Uniform element count, N (elements)", ylabel="Maximum absolute midpoint error (°C)")
    axis.grid(True, which="both", alpha=0.25)
    axis.legend(title=f"Meshes: N={', '.join(str(mesh) for mesh in meshes)} elements")
    figure.savefig(args.results / "thermal_mesh_convergence.png", dpi=180)
    plt.close(figure)
    print(f"Wrote thermal plots to {args.results}")


if __name__ == "__main__":
    main()
