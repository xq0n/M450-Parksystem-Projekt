#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "parking/controller.h"
#include "parking/http_parser.h"

using namespace parking;
static int checks = 0, groups = 0;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
void group(const char* s) { ++groups; printf("[%02d] %s\n", groups, s); }

void inputs() {
  group("TC-01/02/03: plate validation and strict numbers");
  char out[PlateSize];
  CHECK(normalizePlate(" lu 123456 ", out)); CHECK(!strcmp(out, "LU 123456"));
  CHECK(normalizePlate("ZH1", out)); CHECK(normalizePlate("abc123456", out));
  for (const auto* s : {"", " ", "XX!?%&", "L1", "ABCD1", "AB", "AB1234567", "LU12A", "LU\n12", "<script>", "\xff" "12"}) CHECK(!normalizePlate(s, out));
  char tiny[3]; CHECK(!normalizePlate("LU1", tiny, sizeof(tiny)));
  uint32_t n; CHECK(unsignedNumber("4294967295", 0, UINT32_MAX, n)); CHECK(n == UINT32_MAX);
  for (const auto* s : {"", "-1", "+1", " 1", "1x", "4294967296", "99999999999999999999"}) CHECK(!unsignedNumber(s, 0, UINT32_MAX, n));
}
void billing() {
  group("TC-06/07/08: tariff boundaries, daily cap and changes");
  Tariff t;
  CHECK(fee(0, t) == 50); CHECK(fee(1, t) == 50); CHECK(fee(120000, t) == 50);
  CHECK(fee(1800000, t) == 50); CHECK(fee(1800001, t) == 100);
  CHECK(fee(86399999, t) == 1500); CHECK(fee(86400000, t) == 1500);
  CHECK(fee(86400001, t) == 1550); CHECK(fee(172800000, t) == 3000);
  t.unitCents = 10; t.dailyMaxCents = 1500;
  CHECK(fee(86400000, t) == 480); CHECK(fee(86400001, t) == 490);
  t.intervalMinutes = 1440; CHECK(fee(86400000, t) == 10);
  t.intervalMinutes = 7; CHECK(fee(86400000, t) == 1500);
  t.intervalMinutes = 0; CHECK(!t.valid()); CHECK(fee(100, t) == 0);
  Store s; uint32_t v, id; CHECK(s.enter("LU1", 0, v) == Result::Ok);
  CHECK(s.quote("LU1", 1000, id) == Result::Ok);
  t = Tariff{}; t.unitCents = 100;
  CHECK(s.configure(t, 2) == Result::Ok); CHECK(s.due(*s.find("LU1"), 1000) == 100);
  CHECK(s.pay("LU1", id, Method::Card, 1000) == Result::Stale);
  for (uint64_t time = 0, last = 0; time < 3 * 86400000ULL; time += 317777) { uint64_t f = fee(time, t); CHECK(f >= last); last = f; }
}
void lifecycle() {
  group("TC-01/04/05/19: timestamp, duplicates, capacity, separate vehicles");
  Store s; s.capacity = 2; uint32_t v1, v2, dummy;
  CHECK(s.enter("LU123456", 123, v1) == Result::Ok);
  CHECK(s.find("lu 123456")->entered == 123);
  CHECK(s.enter("lu123456", 234, dummy) == Result::Duplicate);
  CHECK(s.enter("LU654321", 456, v2) == Result::Ok); CHECK(v1 != v2);
  CHECK(s.enter("LU9", 999, dummy) == Result::Full); CHECK(s.count() == 2);
  CHECK(s.configure(Tariff{}, 1) == Result::Invalid);
  uint32_t quote;
  CHECK(s.quote("LU123456", 1000, quote) == Result::Ok);
  CHECK(s.pay("LU123456", quote, Method::Card, 1000) == Result::Ok);
  CHECK(!s.find("LU654321")->paid);
  CHECK(s.authorize("LU123456", 1100, dummy) == Result::Ok);
  CHECK(s.finish("LU123456", v2) == Result::Stale);
  CHECK(s.finish("LU123456", v1) == Result::Ok); CHECK(s.count() == 1);
  CHECK(s.enter("LU123456", 1200, dummy) == Result::Ok); CHECK(dummy != v1);
  CHECK(s.finish("LU123456", v1) == Result::Stale); CHECK(s.count() == 2);
  CHECK(s.find("LU654321")->entered == 456);
}
void payments() {
  group("TC-09/10/11/12: confirm, cancel, methods, stale and repeated payments");
  Store s; uint32_t visit, id;
  CHECK(s.pay("LU999999", 1, Method::Card, 0) == Result::NotFound);
  CHECK(s.quote("LU999999", 0, id) == Result::NotFound);
  CHECK(s.enter("LU123456", 0, visit) == Result::Ok);
  CHECK(s.quote("LU123456", 60000, id) == Result::Ok); CHECK(!s.find("LU123456")->paid);
  CHECK(s.cancelPayment("LU123456", id) == Result::Ok);
  CHECK(s.pay("LU123456", id, Method::Card, 60000) == Result::Stale);
  CHECK(s.authorize("LU123456", 60000, visit) == Result::Unpaid);
  CHECK(s.quote("LU123456", 60000, id) == Result::Ok);
  CHECK(s.pay("LU123456", id, Method::None, 60000) == Result::Invalid);
  CHECK(s.pay("LU123456", id, Method::Card, 180001) == Result::Stale);
  CHECK(s.quote("LU123456", 180001, id) == Result::Ok);
  CHECK(s.pay("LU123456", id, Method::Twint, 180002) == Result::Ok);
  const auto* r = s.find("LU123456"); CHECK(r->paidAt == 180002); CHECK(r->method == Method::Twint); CHECK(r->paidCents == 50);
  CHECK(s.pay("LU123456", id, Method::Twint, 180003) == Result::Stale); CHECK(r->paidCents == 50);
  CHECK(s.quote("LU123456", 180003, id) == Result::Ok); CHECK(id == 0);
}
void grace() {
  group("TC-13/14/15/16: paid gate authorization and grace exact boundary");
  Store s; uint32_t v, id;
  CHECK(s.enter("LU1", 0, v) == Result::Ok);
  CHECK(s.authorize("LU1", 100, v) == Result::Unpaid);
  CHECK(s.quote("LU1", 100, id) == Result::Ok); CHECK(s.pay("LU1", id, Method::Card, 100) == Result::Ok);
  auto* r = s.find("LU1"); CHECK(s.eligibility(*r, 900100) == Result::Ok);
  CHECK(s.eligibility(*r, 900101) == Result::Expired); CHECK(s.due(*r, 900101) == 0);
  CHECK(s.authorize("LU1", 900101, v) == Result::Expired);
  CHECK(s.quote("LU1", 1860101, id) == Result::Ok); CHECK(s.pay("LU1", id, Method::Card, 1860102) == Result::Ok);
  CHECK(r->paidCents == 100); CHECK(s.authorize("LU1", 1860103, v) == Result::Ok);
  CHECK(s.authorize("LU1", 1860104, v) == Result::Busy);
  CHECK(s.finish("LU1", v, true) == Result::Ok); CHECK(s.eligibility(*r, 1860105) == Result::Ok);
  // Prior receipt keeps its grace even when the operator changes the tariff.
  Tariff t; t.graceMinutes = 1; CHECK(s.configure(t, 10) == Result::Ok);
  CHECK(r->graceUntil == 2760102);
  CHECK(s.quote("LU1", 80000000, id) == Result::Ok); CHECK(s.pay("LU1", id, Method::Card, 80000000) == Result::Ok);
  CHECK(r->paidCents == 1500); CHECK(s.due(*r, 85000000) == 0);
  CHECK(s.eligibility(*r, 85000000) == Result::Expired);
}
Message request(Op op, uint32_t seq, const char* plate = "") {
  Message m; m.op = op; m.link = 7; m.client = 22; m.server = 11; m.seq = seq; strcpy(m.plate, plate); return m;
}
void protocol() {
  group("Protocol: CRC32, length, version, enum, string and bit corruption");
  CHECK(crc32(reinterpret_cast<const uint8_t*>("123456789"), 9) == 0xcbf43926);
  auto m = request(Op::Entry, 2, "LU 123456"); uint8_t frame[FrameSize]; Message parsed;
  encode(m, frame); CHECK(decode(frame, sizeof(frame), parsed)); CHECK(sameRequest(m, parsed));
  for (size_t i = 0; i < FrameSize; ++i) for (int bit = 0; bit < 8; ++bit) { frame[i] ^= 1 << bit; CHECK(!decode(frame, FrameSize, parsed)); frame[i] ^= 1 << bit; }
  for (size_t i = 0; i < FrameSize; ++i) CHECK(!decode(frame, i, parsed));
  CHECK(!decode(frame, FrameSize + 1, parsed));
  frame[3] = 99; put32(frame + 44, crc32(frame, 44)); CHECK(!decode(frame, FrameSize, parsed));
  encode(m, frame); memset(frame + 28, 'A', PlateSize); put32(frame + 44, crc32(frame, 44)); CHECK(!decode(frame, FrameSize, parsed));
  encode(m, frame); frame[27] = 0; frame[43] = 1; put32(frame + 44, crc32(frame, 44)); CHECK(!decode(frame, FrameSize, parsed));
  uint32_t rng = 12345;
  for (int n = 0; n < 10000; ++n) { for (auto& b : frame) { rng = rng * 1664525u + 1013904223u; b = uint8_t(rng >> 24); } CHECK(!decode(frame, FrameSize, parsed)); }
}
void replay() {
  group("Protocol: cached replies, stale requests, wrong sessions and visit IDs");
  Store s; Service srv(s, 11, 7); Message a;
  CHECK(srv.handle(request(Op::Ping, 1), 0, a));
  auto q = request(Op::Entry, 2, "LU 1"); CHECK(srv.handle(q, 100, a)); CHECK(a.result == Result::Ok);
  uint32_t visit = a.visit;
  CHECK(srv.handle(q, 200, a)); CHECK(a.result == Result::Ok && a.visit == visit); CHECK(s.find("LU1")->entered == 100);
  auto different = q; strcpy(different.plate, "LU 2"); CHECK(srv.handle(different, 200, a)); CHECK(a.result == Result::Stale); CHECK(s.count() == 1);
  CHECK(srv.handle(request(Op::Ping, 3), 300, a)); CHECK(srv.handle(q, 400, a)); CHECK(a.result == Result::Stale);
  q.seq = 4; q.server = 99; CHECK(srv.handle(q, 500, a)); CHECK(a.result == Result::Session);
  q.link = 9; CHECK(!srv.handle(q, 500, a));
}
struct Rig {
  Store store;
  Service service{store, 11, 7};
  Controller controller{22, 7};
  uint32_t now = 0;
  std::vector<Message> sent;
  bool dropRequests = false, dropReplies = false;
  void pump(uint32_t ms) {
    const uint32_t until = now + ms;
    while (now < until) {
      now += 10; Message q, a, decoded; uint8_t bytes[FrameSize];
      if (!controller.poll(now, q)) continue;
      sent.push_back(q); encode(q, bytes); CHECK(decode(bytes, FrameSize, decoded));
      if (dropRequests) continue;
      CHECK(service.handle(decoded, now, a)); encode(a, bytes); CHECK(decode(bytes, FrameSize, decoded));
      if (!dropReplies) controller.receive(decoded, now);
    }
  }
  Rig() { pump(1200); CHECK(controller.ready()); }
  void enter(const char* plate = "LU1") { CHECK(controller.action(Op::Entry, plate, now) == Result::Ok); pump(30); CHECK(controller.entryGate.isOpen()); pump(2600); }
  void pay(const char* plate = "LU1") { uint32_t q; CHECK(store.quote(plate, now, q) == Result::Ok); CHECK(store.pay(plate, q, Method::Card, now) == Result::Ok); }
};
void integration() {
  group("TC-01/09/13/17/19: both firmwares' actual domain/controller over encoded frames");
  Rig r; r.enter(); r.enter("LU2"); r.pay(); r.pay("LU2");
  CHECK(r.controller.action(Op::Authorize, "LU1", r.now) == Result::Ok); r.pump(30);
  CHECK(r.controller.exitGate.isOpen()); CHECK(r.store.count() == 2);
  CHECK(r.controller.action(Op::Entry, "LU3", r.now) == Result::Busy);
  r.pump(2700); CHECK(!r.controller.exitGate.isOpen()); CHECK(r.store.count() == 1);
  CHECK(r.controller.action(Op::Authorize, "LU2", r.now) == Result::Ok); r.pump(2800); CHECK(r.store.count() == 0);
}
void denied() {
  group("TC-02/03/04/05/14: controller never opens on denied operations");
  Rig r; r.store.capacity = 1;
  CHECK(r.controller.action(Op::Entry, "", r.now) == Result::Invalid);
  CHECK(r.controller.action(Op::Entry, "XX!?%&", r.now) == Result::Invalid);
  r.enter();
  for (auto op : {Op::Entry, Op::Authorize}) { CHECK(r.controller.action(op, "LU1", r.now) == Result::Ok); r.pump(30); CHECK(!r.controller.entryGate.isOpen() && !r.controller.exitGate.isOpen()); }
  CHECK(r.controller.action(Op::Entry, "LU2", r.now) == Result::Ok); r.pump(30); CHECK(r.controller.result == Result::Full); CHECK(!r.controller.entryGate.isOpen());
}
void losses() {
  group("TC-18: offline cashier, lost entry ACK, late grant rollback, reconnect");
  Rig r; r.dropReplies = true;
  CHECK(r.controller.action(Op::Entry, "LU1", r.now) == Result::Ok); r.pump(2100);
  CHECK(r.store.count() == 1); CHECK(!r.controller.entryGate.isOpen()); CHECK(r.controller.notice == Notice::Timeout);
  CHECK(r.controller.reconciling());
  CHECK(r.controller.action(Op::Entry, "LU2", r.now) == Result::Busy);
  r.dropReplies = false; r.pump(1200); CHECK(r.store.count() == 0); CHECK(!r.controller.entryGate.isOpen()); CHECK(!r.controller.busy());
  CHECK(!r.controller.reconciling());
  r.enter(); r.pay(); r.dropRequests = true;
  CHECK(r.controller.action(Op::Authorize, "LU1", r.now) == Result::Ok); r.pump(2000);
  CHECK(!r.controller.exitGate.isOpen()); r.dropRequests = false; r.pump(1500);
  CHECK(!r.controller.exitGate.isOpen()); CHECK(!r.store.find("LU1")->exiting);
  CHECK(r.controller.action(Op::Authorize, "LU1", r.now) == Result::Ok); r.pump(30); CHECK(r.controller.exitGate.isOpen());
  r.dropReplies = true; r.pump(4500); CHECK(!r.controller.exitGate.isOpen()); CHECK(r.store.count() == 0);
  r.dropReplies = false; r.pump(1500); CHECK(r.store.count() == 0); CHECK(!r.controller.busy());
}
void retryAndStale() {
  group("Retry before deadline opens once; unrelated and old responses cannot open");
  Rig r; r.dropReplies = true; CHECK(r.controller.action(Op::Entry, "LU1", r.now) == Result::Ok); r.pump(400);
  CHECK(r.store.count() == 1); r.dropReplies = false; r.pump(250); CHECK(r.controller.entryGate.isOpen());
  Message old = r.sent.back(), a; CHECK(r.service.handle(old, r.now, a)); CHECK(!r.controller.receive(a, r.now));
  r.pump(2600); CHECK(!r.controller.entryGate.isOpen()); CHECK(!r.controller.receive(a, r.now));
  Transaction tx; auto q = request(Op::Authorize, 50, "LU 1"); tx.start(q, 100);
  a = q; a.reply = true; a.visit = 7;
  auto wrong = a; wrong.seq++; CHECK(!tx.accept(wrong, 101));
  wrong = a; wrong.server++; CHECK(!tx.accept(wrong, 101));
  wrong = a; wrong.client++; CHECK(!tx.accept(wrong, 101));
  wrong = a; strcpy(wrong.plate, "LU 2"); CHECK(!tx.accept(wrong, 101));
  CHECK(tx.accept(a, 1900)); CHECK(tx.expired); CHECK(!tx.accept(a, 1901));
}
void timeAndRestart() {
  group("Time: millis wrap, 30-day billing, nonblocking gate close and restart epoch");
  Clock c; c.update(0xffffff00); c.update(0x100); CHECK(c.now() == 0x100000100ULL);
  c.advance(30ULL * 86400000); CHECK(c.now() > UINT32_MAX);
  GateTimer g; CHECK(g.open(0xfffffff0, 2500)); CHECK(!g.tick(0x900)); CHECK(g.tick(0x9b4)); CHECK(!g.isOpen());
  CHECK(fee(30ULL * 86400000, Tariff{}) == 45000);
  Rig r; r.enter(); r.pay(); r.dropRequests = true;
  CHECK(r.controller.action(Op::Authorize, "LU1", r.now) == Result::Ok); r.pump(10);
  Store empty; Service restarted(empty, 99, 7); Message a;
  CHECK(restarted.handle(r.controller.pending(), r.now, a)); CHECK(a.result == Result::Session);
  CHECK(r.controller.receive(a, r.now)); CHECK(!r.controller.exitGate.isOpen()); CHECK(!r.controller.ready());
}
HttpRequest parse(const std::string& s) { HttpRequest q; for (char c : s) q.feed(c); return q; }
void http() {
  group("HTTP: partial/oversize requests, strict framing, form encoding and duplicate fields");
  auto q = parse("GET / HTTP/1.1\r\nHost: pico\r\n\r\n"); CHECK(q.state == HttpRequest::Complete); CHECK(!strcmp(q.path, "/"));
  q = parse("GET / HTTP/1.1\r\nHost: pico\r\n"); CHECK(q.state == HttpRequest::Reading);
  auto form = [](const std::string& body) { return parse("POST /pay HTTP/1.1\r\nHost: pico\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body); };
  q = form("plate=LU+123456&method=twint"); char b[32]; CHECK(q.state == HttpRequest::Complete); CHECK(q.arg("plate", b, sizeof(b))); CHECK(!strcmp(b, "LU 123456"));
  for (auto body : {"plate=LU%001", "plate=%xx", "plate=LU1&plate=LU2", "plate=%", "plate=%0A"}) { q = form(body); CHECK(!q.arg("plate", b, sizeof(b))); }
  for (auto headers : {"Content-Length: -1\r\n", "Content-Length: 999999\r\n", "Content-Length: 1\r\nContent-Length: 1\r\n", "Transfer-Encoding: chunked\r\n", "Bad Header\r\n"}) {
    q = parse(std::string("POST / HTTP/1.1\r\n") + headers + "\r\n"); CHECK(q.state == HttpRequest::Bad);
  }
  q = parse("GET / HTTP/1.1\r\nLong: " + std::string(3000, 'x')); CHECK(q.state == HttpRequest::Bad);
  q = parse("POST / HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 5\r\n\r\na=1"); CHECK(q.state == HttpRequest::Reading);
}
int main() {
  inputs(); billing(); lifecycle(); payments(); grace(); protocol(); replay(); integration(); denied(); losses(); retryAndStale(); timeAndRestart(); http();
  printf("PASS: %d groups, %d checks\n", groups, checks);
}
