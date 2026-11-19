#pragma once

namespace parking {
class Barrier {
 public:
  virtual ~Barrier() = default;
  virtual void open() = 0;
  virtual void close() = 0;
};

// Apply controller transitions to the shared barrier. The manual servo test
// can operate the driver independently while the controller remains idle.
class BarrierOutput {
 public:
  explicit BarrierOutput(Barrier& barrier) : barrier_(barrier) {}

  bool update(bool open) {
    if (open == wasOpen_) return false;
    if (open) barrier_.open();
    else barrier_.close();
    wasOpen_ = open;
    return true;
  }

 private:
  Barrier& barrier_;
  bool wasOpen_ = false;
};
} // namespace parking
