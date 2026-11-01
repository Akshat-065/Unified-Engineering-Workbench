#pragma once

#include "hardware/device_adapter.h"

#include <memory>
#include <mutex>
#include <vector>

namespace workbench::hardware {

class MockDeviceAdapter : public DeviceAdapter {
 public:
  DeviceSnapshot discover() const override;
  DeviceSnapshot connect(bool simulate_timeout = false) override;
  DeviceSnapshot disconnect() override;
  TelemetryMessage telemetry() const override;

 protected:
  MockDeviceAdapter(DeviceDescriptor descriptor, std::string telemetry_message);

 private:
  DeviceSnapshot snapshot_locked() const;
  mutable std::mutex mutex_;
  DeviceDescriptor descriptor_;
  ConnectionStatus status_ = ConnectionStatus::Disconnected;
  std::string error_;
  std::string recovery_action_;
  std::string telemetry_message_;
};

class MicrocontrollerAdapter final : public MockDeviceAdapter {
 public: MicrocontrollerAdapter();
};
class FpgaAdapter final : public MockDeviceAdapter {
 public: FpgaAdapter();
};
class JtagDebugAdapter final : public MockDeviceAdapter {
 public: JtagDebugAdapter();
};

std::vector<std::shared_ptr<DeviceAdapter>> make_mock_devices();

}  // namespace workbench::hardware
