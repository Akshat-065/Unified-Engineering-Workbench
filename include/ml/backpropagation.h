#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace workbench::ml {

struct Parameters {
  double w{0.5};
  double b{0.0};
  double v{1.0};
  double c{0.0};
};

struct ForwardPass {
  double hidden_pre_activation{};
  double hidden_activation{};
  double predicted_output{};
  double loss{};
};

struct Gradients {
  double w{};
  double b{};
  double v{};
  double c{};
};

struct GradientCheck {
  Gradients analytical;
  Gradients numerical;
  Gradients absolute_error;
  double epsilon{};
};

struct TrainingPoint {
  std::size_t step{};
  double loss{};
  Parameters parameters;
};

struct TaskResult {
  ForwardPass initial_forward;
  Gradients initial_gradients;
  Parameters after_first_update;
  double loss_after_first_update{};
  Parameters final_parameters;
  double final_loss{};
  GradientCheck gradient_check;
  std::vector<TrainingPoint> training;
};

inline constexpr double kInput = 1.0;
inline constexpr double kTarget = 2.0;
inline constexpr double kLearningRate = 0.1;
inline constexpr double kFiniteDifferenceEpsilon = 1e-6;
inline constexpr std::size_t kSubsequentTrainingSteps = 100;

ForwardPass forward(const Parameters& parameters, double input = kInput,
                    double target = kTarget);
Gradients calculate_gradients(const Parameters& parameters,
                              double input = kInput, double target = kTarget);
Gradients central_difference(const Parameters& parameters, double epsilon,
                             double input = kInput, double target = kTarget);
TaskResult run_task_f();
void write_loss_csv(const std::vector<TrainingPoint>& points,
                    const std::string& path);

}  // namespace workbench::ml
