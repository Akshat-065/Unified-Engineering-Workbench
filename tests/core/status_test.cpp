#include "core/status.h"

#include <gtest/gtest.h>

TEST(ServiceStatus, HasIdentityAndHealthyDefaultState) {
  const auto status = workbench::get_service_status();
  EXPECT_TRUE(status.healthy);
  EXPECT_EQ(status.name, "Unified Engineering Workbench API");
  EXPECT_EQ(status.version, "0.1.0");
}
