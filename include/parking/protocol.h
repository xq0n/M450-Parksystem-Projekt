#pragma once
#include "parking/core.h"

namespace parking {
enum class Op : uint8_t { Hello = 1, Ping, Entry, Authorize, Depart, CancelEntry, CancelExit };
struct Message {
  Op op = Op::Hello;
  bool reply = false;
  Result result = Result::Ok;
  uint32_t link = 0, client = 0, server = 0, seq = 0, visit = 0;
  char plate[PlateSize] = {};
};
constexpr size_t FrameSize = 48;
inline uint32_t crc32(const uint8_t* data, size_t len) {
  uint32_t crc = 0xffffffff;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int j = 0; j < 8; ++j) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1)));
  }
  return ~crc;
}
inline void put32(uint8_t* p, uint32_t v) { for (int i = 0; i < 4; ++i) p[i] = uint8_t(v >> (8 * i)); }
inline uint32_t get32(const uint8_t* p) { uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= uint32_t(p[i]) << (8 * i); return v; }
inline void encode(const Message& m, uint8_t* b) {
  memset(b, 0, FrameSize); b[0] = 'P'; b[1] = 'K'; b[2] = 1; b[3] = uint8_t(m.op);
  b[4] = m.reply; b[5] = uint8_t(m.result);
  put32(b + 8, m.link); put32(b + 12, m.client); put32(b + 16, m.server);
  put32(b + 20, m.seq); put32(b + 24, m.visit); memcpy(b + 28, m.plate, PlateSize);
  put32(b + 44, crc32(b, 44));
}
inline bool decode(const uint8_t* b, size_t len, Message& m) {
  if (len != FrameSize || b[0] != 'P' || b[1] != 'K' || b[2] != 1 || b[3] < 1 || b[3] > 7 ||
      b[4] > 1 || b[5] > uint8_t(Result::Conflict) || b[6] || b[7] || get32(b + 44) != crc32(b, 44)) return false;
  if (!memchr(b + 28, 0, PlateSize)) return false;
  size_t n = 0; while (n < PlateSize && b[28 + n]) ++n;
  for (size_t i = n; i < PlateSize; ++i) if (b[28 + i]) return false;
  m = Message{}; m.op = Op(b[3]); m.reply = b[4]; m.result = Result(b[5]);
  m.link = get32(b + 8); m.client = get32(b + 12); m.server = get32(b + 16);
  m.seq = get32(b + 20); m.visit = get32(b + 24); memcpy(m.plate, b + 28, PlateSize);
  if (!m.client || !m.seq || (!m.reply && m.result != Result::Ok)) return false;
  if (m.op == Op::Hello || m.op == Op::Ping) return m.plate[0] == 0 && m.visit == 0;
  if (!m.reply && (m.op == Op::Entry || m.op == Op::Authorize) && m.visit) return false;
  if ((m.op == Op::Depart || m.op == Op::CancelEntry || m.op == Op::CancelExit) && !m.visit) return false;
  char normalized[PlateSize] = {};
  return normalizePlate(m.plate, normalized) && strcmp(normalized, m.plate) == 0;
}
inline bool sameRequest(const Message& a, const Message& b) {
  uint8_t x[FrameSize], y[FrameSize]; encode(a, x); encode(b, y); return !memcmp(x, y, FrameSize);
}
class Service {
 public:
  Service(VehicleStore& store, uint32_t boot, uint32_t link) : store_(store), boot_(boot), link_(link) {}
  bool handle(const Message& q, uint64_t now, Message& a) {
    if (q.reply || q.link != link_) return false;
    a = q; a.reply = true; a.server = boot_;
    if (q.op == Op::Hello) { a.result = Result::Ok; return true; }
    if (q.server != boot_) { a.result = Result::Session; return true; }
    if (q.client != client_) {
      if (q.op != Op::Ping) { a.result = Result::Session; return true; }
      client_ = q.client; hasCache_ = false;
    }
    if (hasCache_ && q.seq <= last_.seq) {
      if (sameRequest(q, last_)) a = answer_;
      else a.result = Result::Stale;
      return true;
    }
    uint32_t visit = 0;
    switch (q.op) {
      case Op::Ping: break;
      case Op::Entry: a.result = store_.enter(q.plate, now, visit); a.visit = visit; break;
      case Op::Authorize: a.result = store_.authorize(q.plate, now, visit); a.visit = visit; break;
      case Op::Depart: a.result = store_.finish(q.plate, q.visit); break;
      case Op::CancelEntry: a.result = store_.finish(q.plate, q.visit, false, true); break;
      case Op::CancelExit: a.result = store_.finish(q.plate, q.visit, true); break;
      default: a.result = Result::Invalid; break;
    }
    last_ = q; answer_ = a; hasCache_ = true; return true;
  }
 private:
  VehicleStore& store_;
  uint32_t boot_, link_, client_ = 0;
  bool hasCache_ = false;
  Message last_, answer_;
};
// After a business timeout reconcile the SAME request. Late replies cannot open gates.
class Transaction {
 public:
  static constexpr uint32_t RetryMs = 300, DeadlineMs = 1800;
  void start(const Message& m, uint32_t now) { request = m; active = true; expired = false; started_ = now; sent_ = false; }
  bool tick(uint32_t now) {
    if (!active) return false;
    if (uint32_t(now - started_) >= DeadlineMs) expired = true;
    if (!sent_ || uint32_t(now - lastSent_) >= (expired ? 1000u : RetryMs)) { sent_ = true; lastSent_ = now; return true; }
    return false;
  }
  bool accept(const Message& a, uint32_t now) {
    if (!active || !a.reply || a.link != request.link || a.client != request.client || a.seq != request.seq ||
        a.op != request.op || strcmp(a.plate, request.plate)) return false;
    if (request.op != Op::Hello && a.server != request.server && a.result != Result::Session) return false;
    if ((request.op == Op::Depart || request.op == Op::CancelEntry || request.op == Op::CancelExit) && a.visit != request.visit) return false;
    if (uint32_t(now - started_) >= DeadlineMs) expired = true;
    active = false; return true;
  }
  Message request;
  bool active = false, expired = false;
 private:
  bool sent_ = false;
  uint32_t started_ = 0, lastSent_ = 0;
};
} // namespace parking
