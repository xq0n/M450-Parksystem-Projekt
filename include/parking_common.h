#pragma once
// Hardware-independent input, time and gate primitives, shared with host tests.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace parking {
constexpr size_t PlateSize = 16;
inline bool normalizePlate(const char* raw, char* out, size_t size = PlateSize) {
  if (!raw || !out) return false;
  char compact[10] = {};
  size_t n = 0, letters = 0;
  for (size_t i = 0; raw[i]; ++i) {
    if (i >= 32) return false;
    char c = raw[i];
    if (c == ' ' || c == '\t') continue;
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) || n >= 9) return false;
    compact[n++] = c;
  }
  while (letters < n && compact[letters] >= 'A' && compact[letters] <= 'Z') ++letters;
  if (letters < 2 || letters > 3 || n - letters < 1 || n - letters > 6 || size < n + 2) return false;
  for (size_t i = letters; i < n; ++i) if (compact[i] < '0' || compact[i] > '9') return false;
  memcpy(out, compact, letters); out[letters] = ' ';
  memcpy(out + letters + 1, compact + letters, n - letters); out[n + 1] = 0;
  return true;
}
inline bool unsignedNumber(const char* s, uint32_t minimum, uint32_t maximum, uint32_t& value) {
  if (!s || !*s) return false;
  uint64_t v = 0;
  for (size_t i = 0; s[i]; ++i) {
    if (i >= 10 || s[i] < '0' || s[i] > '9') return false;
    v = v * 10 + s[i] - '0'; if (v > maximum) return false;
  }
  if (v < minimum) return false;
  value = static_cast<uint32_t>(v); return true;
}
class Clock {
 public:
  uint64_t update(uint32_t raw) { total_ += static_cast<uint32_t>(raw - last_); last_ = raw; return now(); }
  uint64_t now() const { return total_ + offset_; }
  void advance(uint64_t ms) { offset_ += ms; }
 private:
  uint32_t last_ = 0;
  uint64_t total_ = 0, offset_ = 0;
};
class GateTimer {
 public:
  bool open(uint32_t now, uint32_t hold) { if (opened_) return false; opened_ = true; since_ = now; hold_ = hold; return true; }
  bool tick(uint32_t now) { if (opened_ && static_cast<uint32_t>(now - since_) >= hold_) { opened_ = false; return true; } return false; }
  bool isOpen() const { return opened_; }
 private:
  bool opened_ = false;
  uint32_t since_ = 0, hold_ = 0;
};
} // namespace parking
