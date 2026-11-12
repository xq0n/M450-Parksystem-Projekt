#pragma once

#include <Arduino.h>
#include <Servo.h>
#include "parkhaus_config.h"

namespace hardware {
// One physical servo for both directions. The payment/controller logic decides
// when open() may be called; the motor handling mirrors the supplied Gate.cpp.
class Gate {
 public:
  void begin() {
    servo_.attach(BARRIER_SERVO_PIN);
    servo_.write(BARRIER_CLOSED_ANGLE);
  }
  bool attached() { return servo_.attached(); }
  bool isOpen() const { return isOpen_; }
  void open() {
    servo_.write(BARRIER_OPEN_ANGLE);
    isOpen_ = true;
    openedAt_ = millis();
  }
  void close() {
    servo_.write(BARRIER_CLOSED_ANGLE);
    isOpen_ = false;
  }
  void update() {
    if (isOpen_ && uint32_t(millis() - openedAt_) >= GATE_OPEN_MS) close();
  }
 private:
  Servo servo_;
  bool isOpen_ = false;
  uint32_t openedAt_ = 0;
};
} // namespace hardware
