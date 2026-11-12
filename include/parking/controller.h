#pragma once
#include "parking/protocol.h"

namespace parking {
enum class Notice { Ready, Checking, EntryOpen, ExitOpen, Denied, Timeout, Reconciled, Departed, Restart, Fault };
class Controller {
 public:
  Controller(uint32_t boot, uint32_t link, uint32_t hold = 2500) : boot_(boot), link_(link), hold_(hold) {}
  GateTimer entryGate, exitGate;
  Notice notice = Notice::Ready;
  Result result = Result::Ok;
  bool ready() const { return ready_; }
  bool barrierOpen() const { return entryGate.isOpen() || exitGate.isOpen(); }
  bool reconciling() const { return tx_.active && tx_.expired; }
  bool busy() const { return (tx_.active && tx_.request.op != Op::Ping && tx_.request.op != Op::Hello) || entryGate.isOpen() || exitGate.isOpen() || fault_; }
  uint32_t serverBoot() const { return server_; }
  const Message& pending() const { return tx_.request; }
  Result action(Op op, const char* raw, uint32_t now) {
    char plate[PlateSize] = {};
    if ((op != Op::Entry && op != Op::Authorize) || !normalizePlate(raw, plate)) return Result::Invalid;
    if (busy()) return Result::Busy;
    if (!ready_) return Result::Session;
    start(op, plate, 0, now); notice = Notice::Checking; return Result::Ok;
  }
  bool poll(uint32_t now, Message& outgoing) {
    entryGate.tick(now);
    if (exitGate.tick(now)) start(Op::Depart, departure_.plate, departure_.visit, now);
    if (!tx_.active && !entryGate.isOpen() && !exitGate.isOpen() && !fault_ && uint32_t(now - lastIdle_) >= 1000) {
      start(server_ ? Op::Ping : Op::Hello, "", 0, now);
    }
    bool send = tx_.tick(now);
    if (tx_.active && tx_.expired) {
      if (tx_.request.op == Op::Hello || tx_.request.op == Op::Ping) {
        tx_.active = false; ready_ = false; server_ = 0; lastIdle_ = now; return false;
      }
      notice = Notice::Timeout;
    }
    if (send) outgoing = tx_.request;
    return send;
  }
  bool receive(const Message& a, uint32_t now) {
    if (!tx_.accept(a, now)) return false;
    lastIdle_ = now;
    const bool late = tx_.expired;
    if (a.result == Result::Session) {
      server_ = 0; ready_ = false; notice = Notice::Restart;
      // A server restart loses its RAM store. No old grant is acted upon.
      return true;
    }
    if (a.op == Op::Hello) {
      if (!late && a.result == Result::Ok && a.server) { server_ = a.server; start(Op::Ping, "", 0, now); }
      return true;
    }
    if (a.op == Op::Ping) {
      ready_ = !late && a.result == Result::Ok;
      if (!ready_) server_ = 0;
      return true;
    }
    result = a.result;
    if (a.result != Result::Ok) {
      if (a.op == Op::Depart || a.op == Op::CancelEntry || a.op == Op::CancelExit) { fault_ = true; notice = Notice::Fault; }
      else notice = Notice::Denied;
      return true;
    }
    if (a.op == Op::Entry || a.op == Op::Authorize) {
      if (!a.visit) { fault_ = true; notice = Notice::Fault; return true; }
      if (late) {
        start(a.op == Op::Entry ? Op::CancelEntry : Op::CancelExit, a.plate, a.visit, now);
        notice = Notice::Timeout; return true;
      }
      if (a.op == Op::Entry) { entryGate.open(now, hold_); notice = Notice::EntryOpen; }
      else { departure_ = a; exitGate.open(now, hold_); notice = Notice::ExitOpen; }
    } else if (a.op == Op::Depart) notice = Notice::Departed;
    else notice = Notice::Reconciled;
    return true;
  }
 private:
  void start(Op op, const char* plate, uint32_t visit, uint32_t now) {
    if (seq_ == UINT32_MAX) { fault_ = true; notice = Notice::Fault; return; }
    Message q; q.op = op; q.link = link_; q.client = boot_; q.server = server_; q.seq = ++seq_; q.visit = visit;
    strncpy(q.plate, plate, PlateSize - 1); tx_.start(q, now);
  }
  Transaction tx_;
  Message departure_;
  uint32_t boot_, link_, hold_, server_ = 0, seq_ = 0, lastIdle_ = 0;
  bool ready_ = false, fault_ = false;
};
} // namespace parking
