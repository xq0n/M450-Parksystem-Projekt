#pragma once
#include "parking_common.h"

namespace parking {
// Supplies the plate entered through the web UI (or a future camera adapter).
class PlateInput {
 public:
  virtual ~PlateInput() = default;
  virtual bool read(char* plate, size_t size) const = 0;
};
} // namespace parking
