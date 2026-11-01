#include "ml/backpropagation.h"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>

using namespace workbench::ml;

TEST(Backpropagation, ComputesSpecifiedForwardPassAndGradients) {
  const auto f = forward(Parameters{});
  const auto g = calculate_gradients(Parameters{});
  EXPECT_NEAR(f.hidden_activation, std::tanh(0.5), 1e-15);
  EXPECT_NEAR(f.predicted_output, std::tanh(0.5), 1e-15);
  EXPECT_NEAR(f.loss, 0.5 * std::pow(std::tanh(0.5) - 2.0, 2), 1e-15);
  const double residual = std::tanh(0.5) - 2.0;
  const double expected_w = residual * (1.0 - std::pow(std::tanh(0.5), 2));
  EXPECT_NEAR(g.w, expected_w, 1e-15);
  EXPECT_DOUBLE_EQ(g.b, g.w);
  EXPECT_NEAR(g.v, residual * std::tanh(0.5), 1e-15);
  EXPECT_DOUBLE_EQ(g.c, residual);
}

TEST(Backpropagation, CentralDifferenceMatchesEveryAnalyticalGradient) {
  const auto result = run_task_f();
  for (double error : {result.gradient_check.absolute_error.w,
                       result.gradient_check.absolute_error.b,
                       result.gradient_check.absolute_error.v,
                       result.gradient_check.absolute_error.c}) {
    EXPECT_LE(error, 1e-5);
  }
}

TEST(Backpropagation, OneUpdateIsSimultaneousAndReducesLoss) {
  const Parameters initial{};
  const auto gradients = calculate_gradients(initial);
  const auto result = run_task_f();
  EXPECT_DOUBLE_EQ(result.after_first_update.w, initial.w - kLearningRate * gradients.w);
  EXPECT_DOUBLE_EQ(result.after_first_update.b, initial.b - kLearningRate * gradients.b);
  EXPECT_DOUBLE_EQ(result.after_first_update.v, initial.v - kLearningRate * gradients.v);
  EXPECT_DOUBLE_EQ(result.after_first_update.c, initial.c - kLearningRate * gradients.c);
  EXPECT_LT(result.loss_after_first_update, result.initial_forward.loss);
}

TEST(Backpropagation, TrainingHistoryHasInitialStepAndOneHundredSubsequentSteps) {
  const auto result = run_task_f();
  ASSERT_EQ(result.training.size(), kSubsequentTrainingSteps + 2);
  EXPECT_EQ(result.training.front().step, 0u);
  EXPECT_EQ(result.training[1].step, 1u);
  EXPECT_EQ(result.training.back().step, 101u);
  EXPECT_LT(result.final_loss, result.loss_after_first_update);
  for (std::size_t i = 1; i < result.training.size(); ++i)
    EXPECT_LE(result.training[i].loss, result.training[i - 1].loss);
}

TEST(Backpropagation, RejectsNonfiniteParametersAndInvalidEpsilon) {
  Parameters bad;
  bad.w = std::numeric_limits<double>::infinity();
  EXPECT_THROW(forward(bad), std::invalid_argument);
  EXPECT_THROW(central_difference(Parameters{}, 0.0), std::invalid_argument);
  EXPECT_THROW(central_difference(Parameters{}, std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
}

TEST(Backpropagation, ExportsNumericallyGeneratedLossRows) {
  const auto points = run_task_f().training;
  const auto path = std::filesystem::path("results") / "test_backpropagation_loss.csv";
  std::filesystem::create_directories(path.parent_path());
  ASSERT_NO_THROW(write_loss_csv(points, path.string()));
  {
    std::ifstream input(path);
    std::string header;
    std::getline(input, header);
    EXPECT_EQ(header, "training_step,loss,w,b,v,c");
    std::size_t rows = 0;
    std::string line;
    while (std::getline(input, line)) ++rows;
    EXPECT_EQ(rows, points.size());
  }
  std::filesystem::remove(path);
}
