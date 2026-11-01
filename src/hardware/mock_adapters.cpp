#include "hardware/mock_adapters.h"

#include <utility>

namespace workbench::hardware {

const char* to_string(ConnectionStatus status) noexcept {
  switch (status) {
    case ConnectionStatus::Disconnected: return "disconnected";
    case ConnectionStatus::Connected: return "connected";
    case ConnectionStatus::Error: return "error";
  }
  return "error";
}

MockDeviceAdapter::MockDeviceAdapter(DeviceDescriptor descriptor, std::string telemetry_message)
    : descriptor_(std::move(descriptor)), telemetry_message_(std::move(telemetry_message)) {}

DeviceSnapshot MockDeviceAdapter::snapshot_locked() const {
  return {descriptor_, status_, true, true, error_, recovery_action_, {telemetry_message_, true}};
}

DeviceSnapshot MockDeviceAdapter::discover() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return snapshot_locked();
}

DeviceSnapshot MockDeviceAdapter::connect(bool simulate_timeout) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (simulate_timeout) {
    status_ = ConnectionStatus::Error;
    error_ = "Simulated connection timeout: no response before the adapter deadline.";
    recovery_action_ = "Retry connection; the next simulated attempt will proceed normally.";
  } else {
    status_ = ConnectionStatus::Connected;
    error_.clear();
    recovery_action_.clear();
  }
  return snapshot_locked();
}

DeviceSnapshot MockDeviceAdapter::disconnect() {
  std::lock_guard<std::mutex> lock(mutex_);
  status_ = ConnectionStatus::Disconnected;
  error_.clear();
  recovery_action_.clear();
  return snapshot_locked();
}

TelemetryMessage MockDeviceAdapter::telemetry() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return {telemetry_message_, true};
}

MicrocontrollerAdapter::MicrocontrollerAdapter()
    : MockDeviceAdapter({
          "mcu-pico2", "Raspberry Pi Pico 2 (assumed target)", "Microcontroller board",
          "USB-C for host/power; 3-pad SWD debug footprint",
          "USB VBUS nominal 5 V or VSYS 1.8–5.5 V; board regulates 3.3 V",
          "Board GND; shared reference required for debug signals",
          "VSYS input 1.8–5.5 V; regulated 3.3 V I/O domain", "3.3 V GPIO; not 5 V tolerant",
          "USB CDC serial; SWD to RP2350 via SWDIO/SWCLK/GND",
          "No shift for a verified 3.3 V link; use suitable level translation for other domains",
          "SWD target, not JTAG: adapter drives SWCLK, SWDIO is bidirectional, target returns data",
          "SWD pads carry SWDIO, GND, SWCLK; exact pad order/orientation must be checked on board documentation",
          {"USB CDC console", "RP2350 SWD debug interface", "Simulated heartbeat telemetry"}},
          "Simulated RP2350 heartbeat; no physical MCU was queried.") {}

FpgaAdapter::FpgaAdapter()
    : MockDeviceAdapter({
          "fpga-arty-a7-35", "Digilent Arty A7-35T Rev E.2 (assumed target)", "FPGA development board",
          "J10 Micro-USB host/power; J8 1x6 external JTAG; JA–JD expansion headers",
          "J10 USB nominal 5 V or J13 barrel input 7–15 V DC (board-revision instructions apply)",
          "Arty GND; connect to adapter/host signal ground as required",
          "On-board 5 V, 3.3 V, 1.8 V and FPGA core rails; JA–JD I/O assumed 3.3 V bank",
          "3.3 V on the selected expansion/JTAG domain per Rev E.2 schematic assumption",
          "USB-UART through J10; external 4-wire JTAG through J8",
          "No shift only for verified 3.3 V I/O; external devices at other levels require translation",
          "External JTAG adapter owns TCK/TMS/TDI; FPGA returns TDO; J8 pin 6 is target 3.3 V reference only",
          "Rev E.2 J8 assumed: 1 TMS, 2 TDI, 3 TDO, 4 TCK, 5 GND, 6 VCC3V3; confirm board revision",
          {"USB-UART bridge", "Artix-7 JTAG test access port", "Simulated chain discovery"}},
          "Simulated FPGA JTAG-chain message; no FPGA TAP was accessed.") {}

JtagDebugAdapter::JtagDebugAdapter()
    : MockDeviceAdapter({
          "jtag-jlink-edu-mini", "SEGGER J-Link EDU Mini (assumed adapter)", "JTAG/SWD debug adapter",
          "Micro-USB host; 9-pin Cortex-M target connector (pin 7 absent)",
          "USB-powered nominal 5 V; does not supply target power",
          "Target GND on connector pins 3 and 5; connect to target GND",
          "Fixed 3.3 V target interface (VIF)", "3.3 V target signaling only",
          "USB 2.0 host link; JTAG or SWD target signaling",
          "Not assumed to level-shift; only use with a compatible 3.3 V target interface",
          "Adapter drives TCK/TMS/TDI, samples TDO; VTref is target-provided sense, not power",
          "SEGGER 9-pin target convention: 1 VTref, 2 TMS/SWDIO, 3/5 GND, 4 TCK/SWCLK, 6 TDO/SWO, 8 TDI, 9 NC/TRST option, 10 nRESET; verify probe revision",
          {"3.3 V VTref sensing", "JTAG and SWD signal interface", "Simulated connection diagnostics"}},
          "Simulated probe telemetry; no USB probe or target voltage was detected.") {}

std::vector<std::shared_ptr<DeviceAdapter>> make_mock_devices() {
  return {std::make_shared<MicrocontrollerAdapter>(), std::make_shared<FpgaAdapter>(),
          std::make_shared<JtagDebugAdapter>()};
}

}  // namespace workbench::hardware
