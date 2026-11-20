#pragma once
#include "parking/controller.h"
#include "parking/input.h"

namespace parking {
class TickSource {
 public:
  virtual ~TickSource() = default;
  virtual uint32_t ticks() const = 0;
};

class CashLink {
 public:
  virtual ~CashLink() = default;
  virtual void transmit(const Message& request, bool discover) = 0;
};

// Web and UDP adapters supply input events. Only the adapters know about
// HTTP forms, socket addresses and Arduino types.
class GateStation {
 public:
  GateStation(Controller& controller, TickSource& clock, CashLink& link)
      : controller_(controller), clock_(clock), link_(link) {}

  Result action(Op operation, const char* plate) {
    return controller_.action(operation, plate, clock_.ticks());
  }
  Result action(Op operation, const PlateInput& input) {
    char plate[PlateSize] = {};
    if (!input.read(plate, sizeof(plate))) return Result::Invalid;
    return action(operation, plate);
  }
  bool receive(const Message& response) {
    return controller_.receive(response, clock_.ticks());
  }
  void poll() {
    Message request;
    if (controller_.poll(clock_.ticks(), request)) {
      link_.transmit(request, request.op == Op::Hello || controller_.reconciling());
    }
  }

 private:
  Controller& controller_;
  TickSource& clock_;
  CashLink& link_;
};
} // namespace parking
