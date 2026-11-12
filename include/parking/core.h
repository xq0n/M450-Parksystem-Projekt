#pragma once
#include "parking_common.h"

namespace parking {
constexpr size_t MaxRecords = 32;
enum class Result : uint8_t { Ok, Invalid, Duplicate, Full, NotFound, Unpaid, Expired, Busy, Stale, Session, Conflict };
enum class Method : uint8_t { None, Card, Twint };
struct Tariff {
  uint32_t intervalMinutes = 30, unitCents = 50, dailyMaxCents = 1500, graceMinutes = 15;
  bool valid() const {
    return intervalMinutes >= 1 && intervalMinutes <= 1440 && unitCents >= 1 && unitCents <= 100000 &&
           dailyMaxCents >= unitCents && dailyMaxCents <= 10000000 && graceMinutes >= 1 && graceMinutes <= 1440;
  }
};
inline uint64_t fee(uint64_t duration, const Tariff& t, bool minimum = true) {
  if (!t.valid()) return 0;
  const uint64_t day = 86400000, interval = uint64_t(t.intervalMinutes) * 60000;
  // A maximum is a cap, never a fixed charge for a full day.
  const uint64_t fullDayPrice = ((day + interval - 1) / interval) * t.unitCents;
  const uint64_t cap = fullDayPrice < t.dailyMaxCents ? fullDayPrice : t.dailyMaxCents;
  uint64_t units = (duration % day + interval - 1) / interval;
  if (!duration && minimum) units = 1;
  const uint64_t partial = units * t.unitCents;
  return duration / day * cap + (partial < t.dailyMaxCents ? partial : t.dailyMaxCents);
}
struct Record {
  bool used = false, paid = false, exiting = false;
  char plate[PlateSize] = {};
  uint32_t visit = 0, quote = 0;
  uint64_t entered = 0, paidAt = 0, paidCents = 0, quoteAt = 0, quoteCents = 0, graceUntil = 0;
  Method method = Method::None;
};
class Store {
 public:
  Tariff tariff;
  uint8_t capacity = 10;
  const Record* records() const { return records_; }
  Record* find(const char* raw) {
    char plate[PlateSize]; if (!normalizePlate(raw, plate)) return nullptr;
    for (auto& r : records_) if (r.used && !strcmp(r.plate, plate)) return &r;
    return nullptr;
  }
  size_t count() const { size_t n = 0; for (const auto& r : records_) n += r.used; return n; }
  Result enter(const char* raw, uint64_t now, uint32_t& visit) {
    char plate[PlateSize]; if (!normalizePlate(raw, plate)) return Result::Invalid;
    if (find(plate)) return Result::Duplicate;
    if (count() >= capacity) return Result::Full;
    for (auto& r : records_) if (!r.used) {
      r = Record{}; r.used = true; strcpy(r.plate, plate); r.entered = now;
      r.visit = next(nextVisit_); visit = r.visit; return Result::Ok;
    }
    return Result::Full;
  }
  Result eligibility(const Record& r, uint64_t now) const {
    if (r.exiting) return Result::Busy;
    if (!r.paid) return Result::Unpaid;
    return now <= r.graceUntil ? Result::Ok : Result::Expired;
  }
  uint64_t due(const Record& r, uint64_t now) const {
    if (!r.paid) return fee(now - r.entered, tariff);
    if (now <= r.graceUntil) return 0;
    const uint64_t total = fee(now - r.entered, tariff);
    return total > r.paidCents ? total - r.paidCents : 0; // Preserve the daily cap and credit all receipts.
  }
  Result quote(const char* raw, uint64_t now, uint32_t& id) {
    auto* r = find(raw); if (!r) return Result::NotFound;
    if (r->exiting) return Result::Busy;
    if (r->paid && eligibility(*r, now) == Result::Ok) { id = 0; return Result::Ok; }
    r->quote = next(nextQuote_); r->quoteAt = now; r->quoteCents = due(*r, now); id = r->quote;
    return Result::Ok;
  }
  Result pay(const char* raw, uint32_t quoteId, Method method, uint64_t now) {
    auto* r = find(raw); if (!r) return Result::NotFound;
    if (r->exiting) return Result::Busy;
    if (method != Method::Card && method != Method::Twint) return Result::Invalid;
    if (!quoteId || r->quote != quoteId || now - r->quoteAt > 120000) return Result::Stale;
    r->paidCents += r->quoteCents; r->paidAt = now; r->graceUntil = now + uint64_t(tariff.graceMinutes) * 60000;
    r->paid = true; r->method = method; r->quote = 0; return Result::Ok;
  }
  Result cancelPayment(const char* raw, uint32_t quoteId) {
    auto* r = find(raw); if (!r) return Result::NotFound;
    if (!quoteId || r->quote != quoteId) return Result::Stale;
    r->quote = 0; return Result::Ok;
  }
  Result authorize(const char* raw, uint64_t now, uint32_t& visit) {
    auto* r = find(raw); if (!r) return Result::NotFound;
    auto status = eligibility(*r, now);
    if (status == Result::Ok) { r->exiting = true; r->quote = 0; visit = r->visit; }
    return status;
  }
  Result finish(const char* raw, uint32_t visit, bool cancelExit = false, bool cancelEntry = false) {
    auto* r = find(raw); if (!r) return Result::NotFound;
    if (!visit || r->visit != visit) return Result::Stale;
    if (cancelEntry) {
      if (r->paid || r->exiting) return Result::Conflict;
      *r = Record{}; return Result::Ok;
    }
    if (!r->exiting) return Result::Conflict;
    if (cancelExit) r->exiting = false; else *r = Record{};
    return Result::Ok;
  }
  Result configure(const Tariff& t, uint32_t cap) {
    if (!t.valid() || cap < 1 || cap > MaxRecords || cap < count()) return Result::Invalid;
    tariff = t; capacity = static_cast<uint8_t>(cap); invalidateQuotes(); return Result::Ok;
  }
  void invalidateQuotes() { for (auto& r : records_) r.quote = 0; }
  void reset() { for (auto& r : records_) r = Record{}; } // IDs deliberately never reused.
 private:
  static uint32_t next(uint32_t& n) { if (++n == 0) ++n; return n; }
  Record records_[MaxRecords];
  uint32_t nextVisit_ = 0, nextQuote_ = 0;
};
} // namespace parking
