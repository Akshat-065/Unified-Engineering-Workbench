#include "performance/benchmark.h"

#include <cstdlib>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
std::size_t number(const char* value, const char* option) {
  try { std::size_t used=0; const auto parsed=std::stoull(value,&used); if (used!=std::string(value).size()) throw std::invalid_argument("trailing text"); return static_cast<std::size_t>(parsed); }
  catch (...) { throw std::invalid_argument(std::string("invalid value for ")+option); }
}
}
int main(int argc, char** argv) {
  const auto launch_started=std::chrono::steady_clock::now();
  try {
    workbench::performance::BenchmarkConfig config; std::string output="results/performance/task_d_raw.csv";
    for (int i=1;i<argc;++i) {
      const std::string option=argv[i];
      if (i+1>=argc) throw std::invalid_argument("missing value after " + option);
      const char* value=argv[++i];
      if (option=="--cases") config.case_count=number(value,"--cases");
      else if (option=="--mesh") config.mesh_size=number(value,"--mesh");
      else if (option=="--workers") config.workers=static_cast<unsigned int>(number(value,"--workers"));
      else if (option=="--seed") config.seed=number(value,"--seed");
      else if (option=="--output") output=value;
      else throw std::invalid_argument("unknown option: " + option);
    }
    config.launch_to_ready_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-launch_started).count();
    const auto result=workbench::performance::run_benchmark(config);
    const auto parent=std::filesystem::path(output).parent_path(); if (!parent.empty()) std::filesystem::create_directories(parent);
    workbench::performance::write_raw_csv(result,output);
    std::cout << "Cases: " << config.case_count << ", mesh elements: " << config.mesh_size << ", workers: " << result.effective_workers << '/' << result.logical_cores << " logical CPUs\n"
      << "Median serial: " << result.median_serial_ms << " ms\nMedian parallel: " << result.median_parallel_ms << " ms\n"
      << "Speedup: " << result.speedup << "x; efficiency: " << result.efficiency << "\n"
      << "Maximum serial/parallel temperature difference: " << result.maximum_temperature_difference_c << " C\n"
      << "Launch-to-ready time: " << config.launch_to_ready_ms << " ms (CLI process startup; UI readiness is measured separately by the frontend)\n"
      << "Raw repetitions written to " << output << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception& error) { std::cerr << "Benchmark error: " << error.what() << '\n'; return EXIT_FAILURE; }
}
