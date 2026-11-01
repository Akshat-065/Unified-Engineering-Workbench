# Part 2: steady-state 1D thermal finite elements

## Model and units

The modeled bar has length `L = 1 m`, cross-sectional area `A = 1e-4 m²`, conductivity `k = 10 W/(m K)`, and uniform volumetric heat generation `q = 10000 W/m³`. Its end temperatures are prescribed as `T(0) = 100 °C` and `T(L) = 20 °C`. Material properties and the source are constant. Lateral faces are insulated, so heat flows only along the bar axis. The generated heat per unit length is `qA` in W/m. Temperature differences in °C and K have the same numerical value.

The strong form is

```text
-d/dx(k A dT/dx) = q A
```

For the constant coefficients and prescribed end temperatures, the reference solution is

```text
T_exact(x) = 100 - 80 x/L + [q/(2k)] x(L-x)
```

It is 100 °C at `x=0`, 20 °C at `x=L`, and 185 °C at the midpoint for the given source. The assignment's expression is implemented in parameterized form as the linear interpolation between boundary temperatures plus its quadratic generation term.

## Element equation and assembly

Each uniform two-node element has length `h`. Linear shape functions give constant derivatives within the element. The conductivity matrix and consistent generation load are

```text
K_e = (k A / h) [ [ 1, -1 ], [ -1, 1 ] ]
f_e = (q A h / 2) [ 1, 1 ]ᵀ
```

The implementation computes these entries for each element and adds them into the rows and columns indexed by that element's two global node numbers. Shared nodes receive contributions from both adjacent elements. A dense Gaussian-elimination solver with partial pivoting solves the reduced system; this small 81-node maximum does not justify a third-party FEA package.

Both Dirichlet temperatures are applied by eliminating their columns from the interior equations: for each interior row, the right-hand side is reduced by `K_i0 T(0)` and `K_iN T(L)`. The two endpoint values are then retained exactly in the complete nodal vector. This preserves the interior equations and both constraints.

## Error and convergence

For each element, the temperature at its midpoint is obtained by linear finite element interpolation, the average of the two endpoint nodal values. That value is compared against `T_exact` at the same coordinate. The reported mesh error is the maximum absolute midpoint error, rather than an error computed only at nodes.

Because the exact solution is quadratic, the linear finite element nodal temperatures match the analytical nodal values (up to floating-point roundoff). The midpoint interpolation error for this constant-source problem is `q h²/(8k)`. Consequently, when the element count doubles, the maximum midpoint error should decrease by a factor of four. For the requested meshes, the theoretical errors are:

| Elements N | Nodes | Maximum midpoint error (°C) |
| ---: | ---: | ---: |
| 10 | 11 | 1.25 |
| 20 | 21 | 0.3125 |
| 40 | 41 | 0.078125 |
| 80 | 81 | 0.01953125 |

The solver exports nodal and midpoint values, analytical references, and pointwise absolute errors to `thermal_results.csv`; mesh maximum errors go to `thermal_convergence.csv`. The plotting script only reads those files. It creates an all-mesh temperature plot, one analytical-versus-FEM comparison plot for each mesh, and a log-log mesh convergence plot. Every figure labels axes and units and includes a legend with mesh identification.

## Validation and assumptions

The solver rejects non-finite values, non-positive length, area, conductivity, or heat generation, and a zero-element mesh. The source is required to be positive because the assignment fixes it at `10000 W/m³`; zero-source behavior is outside this assignment case. Boundary temperatures may have any finite values. Uniform meshes and constant cross section, conductivity, and source are assumed. There is no convection, radiation, contact resistance, or transient storage term.

GoogleTest covers boundary enforcement, invalid inputs, node/element counts for all four meshes, analytical nodal agreement and midpoint reference, midpoint error, and refinement behavior. Run with:

```powershell
cmake --build --preset windows-msvc-x64
ctest --test-dir build/windows-msvc-x64 -C Release --output-on-failure
```

## Debugging insight for a technical interview

A tempting boundary-condition shortcut is to replace only the first and last stiffness rows with identity rows. If the corresponding columns are left in the interior equations, prescribed-temperature contributions remain coupled to unknowns and the matrix becomes inconsistent with the constrained problem. The implementation instead moves both known-temperature column contributions into the interior right-hand side before solving. A second useful check is that the nodal solution is exact for this quadratic-source case while midpoint values are not: a test that checks only nodes would miss the interpolation error the assignment asks us to measure.
