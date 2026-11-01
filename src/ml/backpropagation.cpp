#include "ml/backpropagation.h"

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace workbench::ml {
namespace {
void require_finite(double value, const char* name) {
  if (!std::isfinite(value)) throw std::invalid_argument(std::string(name) + " must be finite");
}

void validate(const Parameters& p, double input, double target) {
  require_finite(p.w, "w"); require_finite(p.b, "b");
  require_finite(p.v, "v"); require_finite(p.c, "c");
  require_finite(input, "input"); require_finite(target, "target");
}

Parameters perturb(Parameters p, std::size_t index, double delta) {
  switch (index) {
    case 0: p.w += delta; break;
    case 1: p.b += delta; break;
    case 2: p.v += delta; break;
    case 3: p.c += delta; break;
    default: throw std::logic_error("invalid parameter index");
  }
  return p;
}

void set_component(Gradients& g, std::size_t index, double value) {
  switch (index) {
    case 0: g.w = value; break;
    case 1: g.b = value; break;
    case 2: g.v = value; break;
    default: g.c = value;
  }
}

Parameters updated(const Parameters& p, const Gradients& g) {
  // Compute every component from the same original parameter vector.
  return {p.w - kLearningRate * g.w, p.b - kLearningRate * g.b,
          p.v - kLearningRate * g.v, p.c - kLearningRate * g.c};
}
}  // namespace

ForwardPass forward(const Parameters& p, double input, double target) {
  validate(p, input, target);
  const double z = p.w * input + p.b;
  const double h = std::tanh(z);
  const double prediction = p.v * h + p.c;
  const double residual = prediction - target;
  const double objective = 0.5 * residual * residual;
  if (!std::isfinite(z) || !std::isfinite(h) || !std::isfinite(prediction) ||
      !std::isfinite(residual) || !std::isfinite(objective))
    throw std::overflow_error("forward pass produced a non-finite intermediate or result");
  return {z, h, prediction, objective};
}

Gradients calculate_gradients(const Parameters& p, double input, double target) {
  const auto result = forward(p, input, target);
  const double dJ_dyhat = result.predicted_output - target;
  const double dyhat_dv = result.hidden_activation;
  const double dyhat_dc = 1.0;
  const double dyhat_dh = p.v;
  const double dh_dz = 1.0 - result.hidden_activation * result.hidden_activation;
  const double dz_dw = input;
  const double dz_db = 1.0;
  const Gradients gradients{dJ_dyhat * dyhat_dh * dh_dz * dz_dw,
                            dJ_dyhat * dyhat_dh * dh_dz * dz_db,
                            dJ_dyhat * dyhat_dv,
                            dJ_dyhat * dyhat_dc};
  if (!std::isfinite(gradients.w) || !std::isfinite(gradients.b) ||
      !std::isfinite(gradients.v) || !std::isfinite(gradients.c))
    throw std::overflow_error("gradient calculation produced a non-finite result");
  return gradients;
}

Gradients central_difference(const Parameters& p, double epsilon,
                             double input, double target) {
  validate(p, input, target);
  require_finite(epsilon, "epsilon");
  if (epsilon <= 0.0) throw std::invalid_argument("epsilon must be positive");
  Gradients result;
  for (std::size_t i = 0; i < 4; ++i) {
    const double plus = forward(perturb(p, i, epsilon), input, target).loss;
    const double minus = forward(perturb(p, i, -epsilon), input, target).loss;
    set_component(result, i, (plus - minus) / (2.0 * epsilon));
  }
  return result;
}

TaskResult run_task_f() {
  const Parameters initial{};
  TaskResult result;
  result.initial_forward = forward(initial);
  result.initial_gradients = calculate_gradients(initial);
  const auto numerical = central_difference(initial, kFiniteDifferenceEpsilon);
  result.gradient_check = {result.initial_gradients, numerical,
      {std::abs(result.initial_gradients.w - numerical.w),
       std::abs(result.initial_gradients.b - numerical.b),
       std::abs(result.initial_gradients.v - numerical.v),
       std::abs(result.initial_gradients.c - numerical.c)}, kFiniteDifferenceEpsilon};

  result.training.reserve(kSubsequentTrainingSteps + 2);
  result.training.push_back({0, result.initial_forward.loss, initial});
  result.after_first_update = updated(initial, result.initial_gradients);
  result.loss_after_first_update = forward(result.after_first_update).loss;
  result.training.push_back({1, result.loss_after_first_update, result.after_first_update});

  Parameters current = result.after_first_update;
  for (std::size_t step = 2; step <= kSubsequentTrainingSteps + 1; ++step) {
    const auto gradients = calculate_gradients(current);
    current = updated(current, gradients);
    result.training.push_back({step, forward(current).loss, current});
  }
  result.final_parameters = current;
  result.final_loss = forward(current).loss;
  return result;
}

void write_loss_csv(const std::vector<TrainingPoint>& points,
                    const std::string& path) {
  if (points.empty()) throw std::invalid_argument("cannot export an empty training history");
  std::ofstream output(path);
  if (!output) throw std::runtime_error("cannot open loss CSV for writing: " + path);
  output.precision(17);
  output << "training_step,loss,w,b,v,c\n";
  for (const auto& point : points) {
    output << point.step << ',' << point.loss << ',' << point.parameters.w << ','
           << point.parameters.b << ',' << point.parameters.v << ',' << point.parameters.c << '\n';
  }
  if (!output) throw std::runtime_error("failed while writing loss CSV: " + path);
}

}  // namespace workbench::ml
