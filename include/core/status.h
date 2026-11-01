#pragma once

#include <string>

namespace workbench {

struct ServiceStatus {
  std::string name;
  std::string version;
  bool healthy;
};

ServiceStatus get_service_status();

}  // namespace workbench
