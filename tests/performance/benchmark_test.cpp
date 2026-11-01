#include "performance/benchmark.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

using namespace workbench::performance;

TEST(PerformanceBatch, FixedSeedProducesIdenticalPositiveCases) {
  const auto first=generate_cases(16,9981), second=generate_cases(16,9981);
  ASSERT_EQ(first.size(),second.size());
  for (std::size_t i=0;i<first.size();++i) {
    EXPECT_EQ(first[i].case_id,i);
    EXPECT_DOUBLE_EQ(first[i].parameters.conductivity_w_per_m_k,second[i].parameters.conductivity_w_per_m_k);
    EXPECT_DOUBLE_EQ(first[i].parameters.heat_generation_w_per_m3,second[i].parameters.heat_generation_w_per_m3);
    EXPECT_GT(first[i].parameters.conductivity_w_per_m_k,0);
    EXPECT_GT(first[i].parameters.heat_generation_w_per_m3,0);
  }
}
TEST(PerformanceBatch, SerialAndParallelAreSameAndInInputOrder) {
  const auto cases=generate_cases(7,2026);
  const auto serial=solve_serial(cases,12), parallel=solve_parallel(cases,12,3);
  ASSERT_EQ(serial.size(),parallel.size());
  for (std::size_t i=0;i<serial.size();++i) {
    ASSERT_EQ(serial[i].element_count,parallel[i].element_count);
    EXPECT_EQ(serial[i].nodal_temperatures_c,parallel[i].nodal_temperatures_c);
  }
}
TEST(PerformanceBatch, BenchmarkMeasuresFiveRepetitionsAndReportsMetrics) {
  BenchmarkConfig config; config.case_count=5; config.mesh_size=6; config.workers=3; config.seed=7;
  const auto result=run_benchmark(config);
  ASSERT_EQ(result.repetitions.size(),5u);
  EXPECT_LE(result.maximum_temperature_difference_c,1e-8);
  EXPECT_GT(result.median_serial_ms,0);
  EXPECT_GT(result.median_parallel_ms,0);
  EXPECT_DOUBLE_EQ(result.speedup,result.median_serial_ms/result.median_parallel_ms);
  EXPECT_DOUBLE_EQ(result.efficiency,result.speedup/result.effective_workers);
  EXPECT_LE(result.effective_workers,result.logical_cores);
}
TEST(PerformanceBatch, RejectsInvalidConfig) {
  EXPECT_THROW(generate_cases(0,1),std::invalid_argument);
  BenchmarkConfig invalid; invalid.mesh_size=0; EXPECT_THROW(run_benchmark(invalid),std::invalid_argument);
  BenchmarkConfig excessive; excessive.case_count=40; excessive.mesh_size=256; EXPECT_THROW(run_benchmark(excessive),std::invalid_argument);
  EXPECT_THROW(solve_parallel(generate_cases(1,3),2,0),std::invalid_argument);
}
TEST(PerformanceBatch, ExportRawWorkerComparisons) {
  std::filesystem::create_directories("results/performance");
  for (unsigned int workers=1;workers<=4;++workers) {
    BenchmarkConfig config; config.case_count=40; config.mesh_size=80; config.workers=workers; config.seed=20261101;
    const auto result=run_benchmark(config);
    const auto path="results/performance/task_d_workers_"+std::to_string(workers)+".csv";
    EXPECT_NO_THROW(write_raw_csv(result,path));
  }
}
