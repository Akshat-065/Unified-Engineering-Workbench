#include "core/status.h"
#include "geometry/ascii_stl.h"
#include "geometry/temperature_csv.h"
#include "performance/benchmark.h"
#include "hardware/mock_adapters.h"
#include "ml/backpropagation.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {
using nlohmann::json;

json hardware_json(const workbench::hardware::DeviceSnapshot& snapshot) {
  return {{"id",snapshot.device.id},{"name",snapshot.device.name},
    {"device_type",snapshot.device.device_type},{"connector",snapshot.device.connector},
    {"power_input",snapshot.device.power_input},{"ground",snapshot.device.ground},
    {"voltage_domain",snapshot.device.voltage_domain},{"logic_voltage",snapshot.device.logic_voltage},
    {"communication_protocol",snapshot.device.communication_protocol},{"level_shifting",snapshot.device.level_shifting},
    {"jtag_signal_ownership",snapshot.device.jtag_signal_ownership},{"pinout_assumptions",snapshot.device.pinout_assumptions},
    {"capabilities",snapshot.device.capabilities},{"status",workbench::hardware::to_string(snapshot.status)},
    {"discovered",snapshot.discovered},{"simulated",snapshot.simulated},
    {"error",snapshot.error},{"recovery_action",snapshot.recovery_action},
    {"telemetry",{{"message",snapshot.telemetry.message},{"simulated",snapshot.telemetry.simulated}}}};
}

std::shared_ptr<workbench::hardware::DeviceAdapter> find_hardware_device(
    const std::vector<std::shared_ptr<workbench::hardware::DeviceAdapter>>& devices,
    const std::string& id) {
  for (const auto& device:devices) if (device->discover().device.id==id) return device;
  return {};
}

int read_port() {
  const char* value = std::getenv("WORKBENCH_PORT");
  if (value == nullptr) return 8080;
  try {
    const int port = std::stoi(value);
    if (port >= 1 && port <= 65535) return port;
  } catch (const std::exception&) {
  }
  std::cerr << "Invalid WORKBENCH_PORT; expected an integer from 1 to 65535.\n";
  std::exit(EXIT_FAILURE);
}
}  // namespace

int main() {
  const int port = read_port();
  httplib::Server server;
  const auto hardware_devices=workbench::hardware::make_mock_devices();
  server.set_payload_max_length(workbench::geometry::kMaxGeometryUploadBytes);
  server.Get("/api/health", [](const httplib::Request&, httplib::Response& response) {
    response.set_content(json{{"status", "ok"}, {"healthy", true}}.dump(), "application/json");
  });
  server.Get("/api/status", [](const httplib::Request&, httplib::Response& response) {
    const auto status = workbench::get_service_status();
    response.set_content(json{{"name", status.name}, {"version", status.version},
                              {"healthy", status.healthy}}.dump(), "application/json");
  });
  server.Get("/api/hardware/devices", [hardware_devices](const httplib::Request&, httplib::Response& response) {
    json devices=json::array();
    for (const auto& adapter:hardware_devices) devices.push_back(hardware_json(adapter->discover()));
    response.set_content(json{{"simulated",true},{"devices",devices}}.dump(),"application/json");
  });
  server.Post(R"(/api/hardware/devices/([^/]+)/connect)", [hardware_devices](const httplib::Request& request, httplib::Response& response) {
    try {
      const auto adapter=find_hardware_device(hardware_devices,request.matches[1]);
      if(!adapter){response.status=404;response.set_content(json{{"error","Unknown hardware device id"}}.dump(),"application/json");return;}
      bool simulate_timeout=false;
      if(!request.body.empty()) {
        const auto body=json::parse(request.body);
        if(body.contains("simulate_timeout")) simulate_timeout=body.at("simulate_timeout").get<bool>();
      }
      response.set_content(hardware_json(adapter->connect(simulate_timeout)).dump(),"application/json");
    } catch(const std::exception& error) { response.status=400; response.set_content(json{{"error",error.what()}}.dump(),"application/json"); }
  });
  server.Post(R"(/api/hardware/devices/([^/]+)/disconnect)", [hardware_devices](const httplib::Request& request, httplib::Response& response) {
    const auto adapter=find_hardware_device(hardware_devices,request.matches[1]);
    if(!adapter){response.status=404;response.set_content(json{{"error","Unknown hardware device id"}}.dump(),"application/json");return;}
    response.set_content(hardware_json(adapter->disconnect()).dump(),"application/json");
  });
  server.Get("/api/performance/hardware", [](const httplib::Request&, httplib::Response& response) {
    const auto h = workbench::performance::detect_hardware();
    response.set_content(json{{"cpu_model",h.cpu_model},{"physical_cores",h.physical_cores},
      {"logical_cores",h.logical_cores},{"ram_bytes",h.ram_bytes},{"compiler",h.compiler},
      {"optimization_flags",h.optimization_flags}}.dump(),"application/json");
  });
  server.Post("/api/performance/benchmark", [](const httplib::Request& request, httplib::Response& response) {
    try {
      const auto body=json::parse(request.body);
      workbench::performance::BenchmarkConfig config;
      if (body.contains("case_count")) config.case_count=body.at("case_count").get<std::size_t>();
      if (body.contains("mesh_size")) config.mesh_size=body.at("mesh_size").get<std::size_t>();
      if (body.contains("workers")) config.workers=body.at("workers").get<unsigned int>();
      if (body.contains("seed")) config.seed=body.at("seed").get<std::uint64_t>();
      if (body.contains("launch_to_ready_ms")) config.launch_to_ready_ms=body.at("launch_to_ready_ms").get<double>();
      const auto r=workbench::performance::run_benchmark(config);
      json repetitions=json::array();
      for (const auto& row:r.repetitions) repetitions.push_back({{"repetition",row.repetition},{"serial_ms",row.serial_ms},{"parallel_ms",row.parallel_ms},{"max_temperature_difference_C",row.maximum_temperature_difference_c}});
      response.set_content(json{{"case_count",config.case_count},{"mesh_size",config.mesh_size},{"seed",config.seed},
        {"requested_workers",config.workers},{"workers",r.effective_workers},{"logical_cores",r.logical_cores},
        {"warmup_runs_per_mode",1},{"measured_repetitions",r.repetitions.size()},{"repetitions",repetitions},
        {"median_serial_ms",r.median_serial_ms},{"median_parallel_ms",r.median_parallel_ms},
        {"speedup",r.speedup},{"efficiency",r.efficiency},{"max_temperature_difference_C",r.maximum_temperature_difference_c},
        {"launch_to_ready_ms",config.launch_to_ready_ms},
        {"hardware",{{"cpu_model",r.hardware.cpu_model},{"physical_cores",r.hardware.physical_cores},{"logical_cores",r.hardware.logical_cores},{"ram_bytes",r.hardware.ram_bytes},{"compiler",r.hardware.compiler},{"optimization_flags",r.hardware.optimization_flags}}}}.dump(),"application/json");
    } catch (const std::exception& error) {
      response.status=400; response.set_content(json{{"error",error.what()}}.dump(),"application/json");
    }
  });
  server.Get("/api/ml/backprop", [](const httplib::Request&, httplib::Response& response) {
    const auto r = workbench::ml::run_task_f();
    json history = json::array();
    for (const auto& point : r.training) history.push_back({{"step",point.step},{"loss",point.loss}});
    const json gradients = {{"w",r.initial_gradients.w},{"b",r.initial_gradients.b},
                            {"v",r.initial_gradients.v},{"c",r.initial_gradients.c}};
    const json analytical = {{"w",r.gradient_check.analytical.w},{"b",r.gradient_check.analytical.b},
                             {"v",r.gradient_check.analytical.v},{"c",r.gradient_check.analytical.c}};
    const json numerical = {{"w",r.gradient_check.numerical.w},{"b",r.gradient_check.numerical.b},
                            {"v",r.gradient_check.numerical.v},{"c",r.gradient_check.numerical.c}};
    const json errors = {{"w",r.gradient_check.absolute_error.w},{"b",r.gradient_check.absolute_error.b},
                         {"v",r.gradient_check.absolute_error.v},{"c",r.gradient_check.absolute_error.c}};
    const json initial = {{"w",0.5},{"b",0.0},{"v",1.0},{"c",0.0}};
    const json first = {{"w",r.after_first_update.w},{"b",r.after_first_update.b},
                        {"v",r.after_first_update.v},{"c",r.after_first_update.c}};
    const json final_parameters = {{"w",r.final_parameters.w},{"b",r.final_parameters.b},
                                   {"v",r.final_parameters.v},{"c",r.final_parameters.c}};
    const json check = {{"epsilon",r.gradient_check.epsilon},{"analytical",analytical},
                        {"numerical",numerical},{"absolute_error",errors}};
    const json result = {{"input",workbench::ml::kInput},{"target",workbench::ml::kTarget},
      {"learning_rate",workbench::ml::kLearningRate},{"initial",initial},
      {"hidden_pre_activation",r.initial_forward.hidden_pre_activation},
      {"hidden_activation",r.initial_forward.hidden_activation},
      {"predicted_output",r.initial_forward.predicted_output},{"initial_loss",r.initial_forward.loss},
      {"gradients",gradients},{"after_first_update",first},
      {"loss_after_first_update",r.loss_after_first_update},{"gradient_check",check},
      {"final_loss",r.final_loss},{"final_parameters",final_parameters},
      {"training_steps_after_first_update",workbench::ml::kSubsequentTrainingSteps},
      {"loss_history",history}};
    response.set_content(result.dump(),"application/json");
  });
  server.Post("/api/geometry/stl", [](const httplib::Request& request, httplib::Response& response) {
    try {
      const auto model = workbench::geometry::parse_ascii_stl(request.body);
      nlohmann::json triangles = nlohmann::json::array();
      nlohmann::json positions = nlohmann::json::array();
      nlohmann::json normals = nlohmann::json::array();
      double min_x=INFINITY,min_y=INFINITY,min_z=INFINITY,max_x=-INFINITY,max_y=-INFINITY,max_z=-INFINITY;
      for (const auto& triangle : model.triangles) {
        nlohmann::json vertices = nlohmann::json::array();
        for (const auto& v : triangle.vertices) {
          vertices.push_back({v.x,v.y,v.z}); positions.push_back(v.x); positions.push_back(v.y); positions.push_back(v.z);
          min_x=std::min(min_x,v.x); min_y=std::min(min_y,v.y); min_z=std::min(min_z,v.z);
          max_x=std::max(max_x,v.x); max_y=std::max(max_y,v.y); max_z=std::max(max_z,v.z);
          normals.push_back(triangle.normal.x); normals.push_back(triangle.normal.y); normals.push_back(triangle.normal.z);
        }
        triangles.push_back({{"normal",{triangle.normal.x,triangle.normal.y,triangle.normal.z}}, {"vertices",vertices}});
      }
      response.set_content(nlohmann::json{{"solid_name",model.solid_name},{"triangle_count",model.triangles.size()},
        {"triangles",triangles},{"positions",positions},{"normals",normals},
        {"bounds",{{"min",{min_x,min_y,min_z}},{"max",{max_x,max_y,max_z}}}},
        {"file_size_bytes",request.body.size()}}.dump(),"application/json");
    } catch (const std::exception& error) {
      response.status=400; response.set_content(nlohmann::json{{"error",error.what()}}.dump(),"application/json");
    }
  });
  server.Post("/api/geometry/temperature", [](const httplib::Request& request, httplib::Response& response) {
    try {
      const auto cloud = workbench::geometry::parse_temperature_csv(request.body);
      nlohmann::json nodes=nlohmann::json::array();
      double min_t=INFINITY,max_t=-INFINITY,min_x=INFINITY,min_y=INFINITY,min_z=INFINITY,max_x=-INFINITY,max_y=-INFINITY,max_z=-INFINITY;
      for (const auto& n : cloud.nodes) {
        nodes.push_back({{"node_id",n.node_id},{"x_m",n.x_m},{"y_m",n.y_m},{"z_m",n.z_m},{"temperature_C",n.temperature_C}});
        min_t=std::min(min_t,n.temperature_C); max_t=std::max(max_t,n.temperature_C);
        min_x=std::min(min_x,n.x_m); min_y=std::min(min_y,n.y_m); min_z=std::min(min_z,n.z_m);
        max_x=std::max(max_x,n.x_m); max_y=std::max(max_y,n.y_m); max_z=std::max(max_z,n.z_m);
      }
      response.set_content(nlohmann::json{{"node_count",cloud.nodes.size()},{"nodes",nodes},{"temperature_range_C",{min_t,max_t}},
        {"bounds",{{"min",{min_x,min_y,min_z}},{"max",{max_x,max_y,max_z}}}},
        {"file_size_bytes",request.body.size()}}.dump(),"application/json");
    } catch (const std::exception& error) {
      response.status=400; response.set_content(nlohmann::json{{"error",error.what()}}.dump(),"application/json");
    }
  });
  std::cout << "Workbench API listening on http://127.0.0.1:" << port << '\n';
  if (!server.listen("127.0.0.1", port)) {
    std::cerr << "Unable to start API server on port " << port << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
