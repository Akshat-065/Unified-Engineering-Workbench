#include "hardware/mock_adapters.h"

#include <gtest/gtest.h>

#include <algorithm>

using namespace workbench::hardware;

TEST(MockHardware, DiscoversThreeDistinctSimulatedDeviceTypes) {
  const auto devices=make_mock_devices();
  ASSERT_EQ(devices.size(),3u);
  std::vector<std::string> types;
  for(const auto& adapter:devices) {
    const auto state=adapter->discover();
    EXPECT_TRUE(state.discovered);
    EXPECT_TRUE(state.simulated);
    EXPECT_FALSE(state.device.capabilities.empty());
    EXPECT_TRUE(adapter->telemetry().simulated);
    types.push_back(state.device.device_type);
  }
  EXPECT_NE(std::find(types.begin(),types.end(),"Microcontroller board"),types.end());
  EXPECT_NE(std::find(types.begin(),types.end(),"FPGA development board"),types.end());
  EXPECT_NE(std::find(types.begin(),types.end(),"JTAG/SWD debug adapter"),types.end());
}

TEST(MockHardware, ConnectDisconnectAndTelemetryAreAvailable) {
  MicrocontrollerAdapter adapter;
  EXPECT_EQ(adapter.discover().status,ConnectionStatus::Disconnected);
  const auto connected=adapter.connect();
  EXPECT_EQ(connected.status,ConnectionStatus::Connected);
  EXPECT_FALSE(connected.telemetry.message.empty());
  EXPECT_TRUE(connected.telemetry.simulated);
  EXPECT_EQ(adapter.disconnect().status,ConnectionStatus::Disconnected);
}

TEST(MockHardware, ConnectionTimeoutCanRecoverOnRetry) {
  FpgaAdapter adapter;
  const auto failed=adapter.connect(true);
  EXPECT_EQ(failed.status,ConnectionStatus::Error);
  EXPECT_NE(failed.error.find("timeout"),std::string::npos);
  EXPECT_FALSE(failed.recovery_action.empty());
  const auto recovered=adapter.connect(false);
  EXPECT_EQ(recovered.status,ConnectionStatus::Connected);
  EXPECT_TRUE(recovered.error.empty());
  EXPECT_TRUE(recovered.recovery_action.empty());
}
