#pragma once

#include "thermal/solver.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace workbench::performance {

inline constexpr std::size_t kMeasuredRepetitions = 5;
inline constexpr double kMaximumTemperatureDifferenceC = 1e-8;

struct BenchmarkConfig {
  std::size_t case_count = 40;
  std::size_t mesh_size = 80;
  unsigned int workers = 2;
  std::uint64_t seed = 20261101;
  double launch_to_ready_ms = 0.0;
};
struct CaseInput { std::size_t case_id{}; thermal::Parameters parameters; };
struct HardwareInfo {
  std::string cpu_model;
  unsigned int physical_cores{};
  unsigned int logical_cores{};
  std::uint64_t ram_bytes{};
  std::string compiler;
  std::string optimization_flags;
};
struct Repetition { std::size_t repetition{}; double serial_ms{}; double parallel_ms{}; double maximum_temperature_difference_c{}; };
struct BenchmarkResult {
  BenchmarkConfig config;
  unsigned int effective_workers{};
  unsigned int logical_cores{};
  HardwareInfo hardware;
  std::vector<Repetition> repetitions;
  double median_serial_ms{};
  double median_parallel_ms{};
  double speedup{};
  double efficiency{};
  double maximum_temperature_difference_c{};
};

HardwareInfo detect_hardware();
std::vector<CaseInput> generate_cases(std::size_t case_count, std::uint64_t seed);
std::vector<thermal::MeshResult> solve_serial(const std::vector<CaseInput>& cases, std::size_t mesh_size);
std::vector<thermal::MeshResult> solve_parallel(const std::vector<CaseInput>& cases, std::size_t mesh_size, unsigned int workers);
BenchmarkResult run_benchmark(const BenchmarkConfig& config);
void write_raw_csv(const BenchmarkResult& result, const std::string& output_path);

}  // namespace workbench::performance
