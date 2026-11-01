#include "performance/benchmark.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winreg.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#endif

namespace workbench::performance {
namespace {
unsigned int logical_cpu_count() {
  const auto count = std::thread::hardware_concurrency();
  return count == 0 ? 1u : count;
}
double median(std::vector<double> values) {
  std::sort(values.begin(), values.end());
  const auto n = values.size();
  return n % 2 ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2.0;
}
double elapsed_ms(const std::chrono::steady_clock::time_point start,
                  const std::chrono::steady_clock::time_point finish) {
  return std::chrono::duration<double, std::milli>(finish - start).count();
}
std::string csv_quote(const std::string& value) {
  std::string escaped="\""; escaped.reserve(value.size()+2);
  for (const char c:value) { if(c=='"') escaped += "\"\""; else escaped += c; }
  escaped += '"'; return escaped;
}
double compare_results(const std::vector<thermal::MeshResult>& left,
                       const std::vector<thermal::MeshResult>& right) {
  if (left.size() != right.size()) throw std::runtime_error("serial and parallel result counts differ");
  double maximum = 0.0;
  for (std::size_t i = 0; i < left.size(); ++i) {
    if (left[i].element_count != right[i].element_count || left[i].nodal_temperatures_c.size() != right[i].nodal_temperatures_c.size() || left[i].midpoints.size() != right[i].midpoints.size()) throw std::runtime_error("serial and parallel results are not structurally aligned");
    for (std::size_t j = 0; j < left[i].nodal_temperatures_c.size(); ++j) maximum = std::max(maximum, std::abs(left[i].nodal_temperatures_c[j] - right[i].nodal_temperatures_c[j]));
    for (std::size_t j = 0; j < left[i].midpoints.size(); ++j) maximum = std::max(maximum, std::abs(left[i].midpoints[j].finite_element_temperature_c - right[i].midpoints[j].finite_element_temperature_c));
  }
  if (!std::isfinite(maximum) || maximum > kMaximumTemperatureDifferenceC) throw std::runtime_error("serial/parallel temperature difference exceeds 1e-8 C");
  return maximum;
}
}

HardwareInfo detect_hardware() {
  HardwareInfo info; info.logical_cores = logical_cpu_count();
#ifdef _WIN32
  char cpu[512]{}; DWORD cpu_size = sizeof(cpu);
  if (RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString", RRF_RT_REG_SZ, nullptr, cpu, &cpu_size) == ERROR_SUCCESS) info.cpu_model = cpu;
  DWORD bytes = 0; GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes);
  if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && bytes > 0) {
    std::vector<unsigned char> buffer(bytes);
    if (GetLogicalProcessorInformationEx(RelationProcessorCore, reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data()), &bytes)) {
      unsigned int count = 0; for (DWORD offset = 0; offset < bytes;) { auto* entry = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data() + offset); ++count; offset += entry->Size; }
      info.physical_cores = count;
    }
  }
  MEMORYSTATUSEX memory{}; memory.dwLength = sizeof(memory); if (GlobalMemoryStatusEx(&memory)) info.ram_bytes = memory.ullTotalPhys;
#elif defined(__linux__)
  {
    std::ifstream cpuinfo("/proc/cpuinfo"); std::string line, physical, core; std::set<std::string> cores;
    while (std::getline(cpuinfo, line)) {
      auto assign=[&](const char* key,std::string& value){ const std::string prefix=std::string(key)+"\t:"; if(line.rfind(prefix,0)==0) value=line.substr(prefix.size()); };
      const auto model=line.find("model name"); if(model!=std::string::npos && info.cpu_model.empty()){const auto colon=line.find(':',model);if(colon!=std::string::npos)info.cpu_model=line.substr(colon+2);}
      assign("physical id",physical); assign("core id",core);
      if(line.empty() && !physical.empty() && !core.empty()) { cores.insert(physical+":"+core); physical.clear(); core.clear(); }
    }
    if(!physical.empty()&&!core.empty()) cores.insert(physical+":"+core);
    info.physical_cores=static_cast<unsigned int>(cores.size());
    std::ifstream meminfo("/proc/meminfo"); while (std::getline(meminfo, line)) if (line.rfind("MemTotal:", 0) == 0) { std::uint64_t kb{}; std::istringstream(line.substr(9)) >> kb; info.ram_bytes = kb * 1024; break; }
  }
#else
  info.cpu_model = "Unavailable on this platform"; info.physical_cores = info.logical_cores;
#endif
  if (info.cpu_model.empty()) info.cpu_model = "Unavailable";
  if (!info.physical_cores) info.physical_cores = info.logical_cores;
  if (!info.ram_bytes) {
#ifdef _WIN32
    MEMORYSTATUSEX memory{}; memory.dwLength = sizeof(memory); if (GlobalMemoryStatusEx(&memory)) info.ram_bytes = memory.ullTotalPhys;
#endif
  }
#if defined(_MSC_VER)
  info.compiler = std::string("MSVC ") + std::to_string(_MSC_VER/100) + "." + std::to_string(_MSC_VER%100);
#if defined(NDEBUG)
  info.optimization_flags = "CMake Release defaults: /O2 /Ob2 /DNDEBUG";
#else
  info.optimization_flags = "Non-Release configuration; optimization flags are toolchain-specific";
#endif
#elif defined(__clang__)
  info.compiler = std::string("Clang ") + __clang_version__;
#if defined(NDEBUG)
  info.optimization_flags = "CMake Release defaults: -O3 -DNDEBUG";
#else
  info.optimization_flags = "Non-Release configuration; optimization flags are toolchain-specific";
#endif
#elif defined(__GNUC__)
  info.compiler = std::string("GCC ") + __VERSION__;
#if defined(NDEBUG)
  info.optimization_flags = "CMake Release defaults: -O3 -DNDEBUG";
#else
  info.optimization_flags = "Non-Release configuration; optimization flags are toolchain-specific";
#endif
#else
  info.compiler = "Unknown C++17 compiler"; info.optimization_flags = "Build configuration unavailable";
#endif
  return info;
}

std::vector<CaseInput> generate_cases(std::size_t case_count, std::uint64_t seed) {
  if (!case_count || case_count > 100000) throw std::invalid_argument("case_count must be between 1 and 100000");
  std::mt19937_64 generator(seed);
  std::uniform_real_distribution<double> conductivity(5.0, 25.0);
  std::uniform_real_distribution<double> generation(5000.0, 15000.0);
  std::vector<CaseInput> cases; cases.reserve(case_count);
  for (std::size_t i = 0; i < case_count; ++i) {
    thermal::Parameters p; p.conductivity_w_per_m_k = conductivity(generator); p.heat_generation_w_per_m3 = generation(generator);
    cases.push_back({i, p});
  }
  return cases;
}

std::vector<thermal::MeshResult> solve_serial(const std::vector<CaseInput>& cases, std::size_t mesh_size) {
  std::vector<thermal::MeshResult> results; results.reserve(cases.size());
  for (const auto& input : cases) results.push_back(thermal::solve(input.parameters, mesh_size));
  return results;
}

std::vector<thermal::MeshResult> solve_parallel(const std::vector<CaseInput>& cases, std::size_t mesh_size, unsigned int workers) {
  if (!mesh_size) throw std::invalid_argument("mesh_size must be at least one");
  if (!workers) throw std::invalid_argument("workers must be at least one");
  const auto bounded_workers = std::min<unsigned int>({workers, logical_cpu_count(), static_cast<unsigned int>(std::max<std::size_t>(1, cases.size()))});
  std::vector<thermal::MeshResult> results(cases.size()); if (cases.empty()) return results;
  std::atomic<std::size_t> next{0}; std::atomic<bool> stop{false}; std::exception_ptr failure; std::mutex failure_mutex;
  std::vector<std::thread> pool; pool.reserve(bounded_workers);
  try { for (unsigned int w = 0; w < bounded_workers; ++w) pool.emplace_back([&] {
    while (!stop.load(std::memory_order_relaxed)) {
      const auto index = next.fetch_add(1, std::memory_order_relaxed); if (index >= cases.size()) break;
      try { results[index] = thermal::solve(cases[index].parameters, mesh_size); }
      catch (...) { std::lock_guard<std::mutex> lock(failure_mutex); if (!failure) failure = std::current_exception(); stop.store(true, std::memory_order_relaxed); }
    }
  }); } catch (...) { stop.store(true, std::memory_order_relaxed); for(auto& thread:pool) if(thread.joinable()) thread.join(); throw; }
  for (auto& thread : pool) thread.join();
  if (failure) std::rethrow_exception(failure);
  return results;
}

BenchmarkResult run_benchmark(const BenchmarkConfig& config) {
  if (!config.case_count || config.case_count > 100000) throw std::invalid_argument("case_count must be between 1 and 100000");
  if (!config.mesh_size || config.mesh_size > 512) throw std::invalid_argument("mesh_size must be between 1 and 512 for the current dense thermal solver");
  if (static_cast<long double>(config.case_count) * config.mesh_size * config.mesh_size * config.mesh_size > 250000000.0L) throw std::invalid_argument("batch workload exceeds the supported case_count * mesh_size^3 budget of 250000000");
  if (!config.workers) throw std::invalid_argument("workers must be at least one");
  if (!std::isfinite(config.launch_to_ready_ms) || config.launch_to_ready_ms < 0.0) throw std::invalid_argument("launch_to_ready_ms must be finite and non-negative");
  const auto cases = generate_cases(config.case_count, config.seed);
  const unsigned int logical = logical_cpu_count();
  const unsigned int effective = std::min<unsigned int>({config.workers, logical, static_cast<unsigned int>(config.case_count)});
  (void)solve_serial(cases, config.mesh_size); (void)solve_parallel(cases, config.mesh_size, effective); // one warm-up per mode
  BenchmarkResult result; result.config=config; result.effective_workers=effective; result.logical_cores=logical; result.hardware=detect_hardware();
  double maximum_difference=0.0;
  for (std::size_t repetition=0; repetition<kMeasuredRepetitions; ++repetition) {
    auto start=std::chrono::steady_clock::now(); auto serial=solve_serial(cases,config.mesh_size); auto finish=std::chrono::steady_clock::now(); const auto serial_ms=elapsed_ms(start,finish);
    start=std::chrono::steady_clock::now(); auto parallel=solve_parallel(cases,config.mesh_size,effective); finish=std::chrono::steady_clock::now(); const auto parallel_ms=elapsed_ms(start,finish);
    const auto difference=compare_results(serial,parallel); maximum_difference=std::max(maximum_difference,difference);
    result.repetitions.push_back({repetition+1,serial_ms,parallel_ms,difference});
  }
  std::vector<double> serial_times,parallel_times; for (const auto& row:result.repetitions) { serial_times.push_back(row.serial_ms); parallel_times.push_back(row.parallel_ms); }
  result.median_serial_ms=median(serial_times); result.median_parallel_ms=median(parallel_times);
  result.speedup=result.median_parallel_ms > 0.0 ? result.median_serial_ms/result.median_parallel_ms : 0.0;
  result.efficiency=result.effective_workers ? result.speedup/result.effective_workers : 0.0;
  result.maximum_temperature_difference_c=maximum_difference;
  return result;
}

void write_raw_csv(const BenchmarkResult& r, const std::string& output_path) {
  if (output_path.empty()) throw std::invalid_argument("output path must not be empty");
  std::ofstream out(output_path, std::ios::binary); if (!out) throw std::runtime_error("unable to open benchmark CSV output: " + output_path);
  out << "repetition,case_count,mesh_elements,workers,logical_cores,seed,launch_to_ready_ms,serial_ms,parallel_ms,max_temperature_difference_C,median_serial_ms,median_parallel_ms,speedup,efficiency,cpu_model,physical_cores,ram_bytes,compiler,optimization_flags\n";
  out << std::setprecision(17);
  for (const auto& row:r.repetitions) {
    out << row.repetition << ',' << r.config.case_count << ',' << r.config.mesh_size << ','
        << r.effective_workers << ',' << r.logical_cores << ',' << r.config.seed << ',';
    if (r.config.launch_to_ready_ms > 0.0) out << r.config.launch_to_ready_ms;
    out << ',' << row.serial_ms << ',' << row.parallel_ms << ','
        << row.maximum_temperature_difference_c << ',' << r.median_serial_ms << ','
        << r.median_parallel_ms << ',' << r.speedup << ',' << r.efficiency << ','
        << csv_quote(r.hardware.cpu_model) << ',' << r.hardware.physical_cores << ','
        << r.hardware.ram_bytes << ',' << csv_quote(r.hardware.compiler) << ','
        << csv_quote(r.hardware.optimization_flags) << '\n';
  }
  if (!out) throw std::runtime_error("failed while writing benchmark CSV: " + output_path);
}
}  // namespace workbench::performance
