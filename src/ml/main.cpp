#include "ml/backpropagation.h"

#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  try {
    const std::string path = argc > 1 ? argv[1] : "results/backpropagation_loss.csv";
    const auto result = workbench::ml::run_task_f();
    workbench::ml::write_loss_csv(result.training, path);
    std::cout << std::setprecision(12)
      << "Initial: h=" << result.initial_forward.hidden_activation
      << ", y_hat=" << result.initial_forward.predicted_output
      << ", loss=" << result.initial_forward.loss << '\n'
      << "Gradients: dw=" << result.initial_gradients.w << ", db=" << result.initial_gradients.b
      << ", dv=" << result.initial_gradients.v << ", dc=" << result.initial_gradients.c << '\n'
      << "After first update: w=" << result.after_first_update.w << ", b=" << result.after_first_update.b
      << ", v=" << result.after_first_update.v << ", c=" << result.after_first_update.c
      << ", loss=" << result.loss_after_first_update << '\n'
      << "After 100 subsequent updates: w=" << result.final_parameters.w << ", b=" << result.final_parameters.b
      << ", v=" << result.final_parameters.v << ", c=" << result.final_parameters.c
      << ", loss=" << result.final_loss << '\n'
      << "Finite difference absolute errors: dw=" << result.gradient_check.absolute_error.w
      << ", db=" << result.gradient_check.absolute_error.b << ", dv=" << result.gradient_check.absolute_error.v
      << ", dc=" << result.gradient_check.absolute_error.c << '\n'
      << "Wrote " << result.training.size() << " loss rows to " << path << '\n';
    for (double error : {result.gradient_check.absolute_error.w, result.gradient_check.absolute_error.b,
                         result.gradient_check.absolute_error.v, result.gradient_check.absolute_error.c}) {
      if (error > 1e-5 || !std::isfinite(error)) return 2;
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Task F failed: " << error.what() << '\n';
    return 1;
  }
}
