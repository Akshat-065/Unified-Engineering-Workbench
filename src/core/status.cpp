#include "core/status.h"

namespace workbench {

ServiceStatus get_service_status() {
  return {"Unified Engineering Workbench API", "0.1.0", true};
}

}  // namespace workbench
