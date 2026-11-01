# Task F: Hand-Derived Backpropagation

This note describes the C++ implementation and provides a concise guide for preparing the required handwritten derivation. The generated calculations and this typed guide are not handwritten evidence.

## Model and forward propagation

The single training sample is `x = 1`, target `y = 2`, and learning rate `eta = 0.1`. Parameters start at `(w,b,v,c) = (0.5,0,1,0)`. The hidden and output equations are

```text
z = wx + b
h = tanh(z)
y_hat = vh + c
J = 0.5 (y_hat - y)^2
```

At initialization:

```text
z = 0.5(1) + 0 = 0.5
h = tanh(0.5) = 0.4621171572600098
y_hat = 1(0.4621171572600098) + 0 = 0.4621171572600098
e = y_hat - y = -1.5378828427399902
J = 0.5 e^2 = 1.182541818997...
```

## Chain-rule derivation

Start at the loss and work backward:

```text
dJ/dy_hat = y_hat - y = e = -1.5378828427399902
dy_hat/dv = h = 0.4621171572600098
dy_hat/dc = 1
dy_hat/dh = v = 1
dh/dz = 1 - tanh(z)^2 = 1 - h^2 = 0.7864477329659274
dz/dw = x = 1
dz/db = 1
```

Combining the local derivatives:

```text
dJ/dv = (dJ/dy_hat)(dy_hat/dv)
      = e h = -0.710682047486...
dJ/dc = (dJ/dy_hat)(dy_hat/dc)
      = e = -1.537882842740...
dJ/dw = (dJ/dy_hat)(dy_hat/dh)(dh/dz)(dz/dw)
      = e v (1 - h^2) x = -1.20946447524...
dJ/db = (dJ/dy_hat)(dy_hat/dh)(dh/dz)(dz/db)
      = e v (1 - h^2) = -1.20946447524...
```

This explicitly shows how the output error flows through `v`, then through the tanh derivative, to the hidden layer parameters `w` and `b`.

## Simultaneous gradient descent update

Every gradient above is computed from the original parameter vector. Then all four parameters are updated together using `theta_new = theta_old - eta * gradient_old`:

| Parameter | Initial | Initial gradient | After first update |
| --- | ---: | ---: | ---: |
| `w` | 0.5 | -1.20946447524 | 0.620946447524 |
| `b` | 0 | -1.20946447524 | 0.120946447524 |
| `v` | 1 | -0.710682047486 | 1.071068204749 |
| `c` | 0 | -1.537882842740 | 0.153788284274 |

The initial loss is `1.182541818997...`; after this first update it is `0.685773765045...`. A negative gradient means increasing that parameter locally increases the loss, so subtracting the negative gradient moves it upward and lowers the objective here. The update uses no parameter values modified partway through the calculation.

## Finite-difference verification

For each parameter independently, the C++ program evaluates the loss at the original parameter vector plus and minus `epsilon = 1e-6` in that coordinate and computes

```text
g_numeric = [J(theta + epsilon) - J(theta - epsilon)] / (2 epsilon)
absolute_error = abs(g_numeric - g_analytical)
```

The recorded absolute errors from the Release C++ execution are approximately `1.13e-10` for `w`, `1.13e-10` for `b`, `5.25e-11` for `v`, and `1.12e-10` for `c`, each below the required `1e-5`. They are runtime-generated values; tests check the tolerance rather than hardcoding those measurements.

## Training and artifacts

After the specifically requested first update, training performs 100 further simultaneous updates at the same learning rate. Thus exported history contains 102 rows including step 0: initial state, step 1 after the first update, and steps 2 through 101 after the 100 subsequent updates. The computed loss decreased monotonically in this run, from `1.182541819...` initially, to `0.685773765...` after step 1, and to approximately `1.08e-20` at step 101. This behavior is specific to this sample, initialization, and learning rate; it is not a general guarantee for gradient descent.

Run from the repository root:

```powershell
cmake --build build/vs18-x64 --config Release --target workbench_ml
& .\build\vs18-x64\Release\workbench_ml.exe .\results\backpropagation_loss.csv
python scripts/plot_backpropagation_loss.py --input .\results\backpropagation_loss.csv --output .\results\backpropagation_loss.svg
```

The C++ executable generates the numeric CSV; the Python standard-library script only plots the already computed values as a labelled SVG. No plotting package is required for this chart. The frontend Backpropagation page receives model results from `GET /api/ml/backprop` and draws an SVG chart for display; it performs no model calculation.

## Two-page handwritten preparation guide

Suggested layout for a maximum two-page handwritten appendix:

1. **Page 1 — forward pass and graph derivatives.** Write the model equations, substitute `x=1`, compute `z`, `h`, `y_hat`, `e`, and `J`. Then write the six local derivatives from `J` back to `w,b,v,c`, including `1-h^2` for tanh.
2. **Page 2 — parameter gradients, update, and check.** Show each of the four chain-rule products. Copy the initial parameter/gradient/update table, explicitly state all gradients use the original parameters, and show before/after loss. Finish with the central-difference formula, `epsilon=1e-6`, and the `1e-5` acceptance threshold.

Use the values in this derivation as a checking reference while writing by hand. The appendix must be handwritten separately; the project does not represent this document or generated plots as handwritten evidence.

## Implementation and limitations

The C++17 core in `include/ml/backpropagation.h` and `src/ml/backpropagation.cpp` implements the forward pass, analytic derivatives, central differences, simultaneous updates, validation against nonfinite values, and CSV export without autodiff or an ML framework. `workbench_ml` is a small CLI around that reusable core. The API endpoint exposes the computed values to React. Unit tests cover the exact gradients, finite-difference tolerance, one simultaneous update, monotonic loss for this fixed run, invalid inputs, and CSV contents.
