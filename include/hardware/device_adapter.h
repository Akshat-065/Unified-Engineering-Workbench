#pragma once

#include <string>
#include <vector>

namespace workbench::hardware {

enum class ConnectionStatus { Disconnected, Connected, Error };

struct DeviceDescriptor {
  std::string id;
  std::string name;
  std::string device_type;
  std::string connector;
  std::string power_input;
  std::string ground;
  std::string voltage_domain;
  std::string logic_voltage;
  std::string communication_protocol;
  std::string level_shifting;
  std::string jtag_signal_ownership;
  std::string pinout_assumptions;
  std::vector<std::string> capabilities;
};

struct TelemetryMessage {
  std::string message;
  bool simulated = true;
};

struct DeviceSnapshot {
  DeviceDescriptor device;
  ConnectionStatus status = ConnectionStatus::Disconnected;
  bool discovered = true;
  bool simulated = true;
  std::string error;
  std::string recovery_action;
  TelemetryMessage telemetry;
};

class DeviceAdapter {
 public:
  virtual ~DeviceAdapter() = default;
  virtual DeviceSnapshot discover() const = 0;
  virtual DeviceSnapshot connect(bool simulate_timeout = false) = 0;
  virtual DeviceSnapshot disconnect() = 0;
  virtual TelemetryMessage telemetry() const = 0;
};

const char* to_string(ConnectionStatus status) noexcept;

}  // namespace workbench::hardware
