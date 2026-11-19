#include <Arduino.h>
#include "parking/controller.h"
#include "drivers/runtime.h"
#include "drivers/http_server.h"
#include "drivers/gate.h"
#include "web/ui.h"

namespace {
hardware::Network network;
hardware::Datagram link;
parking::Controller* controller = nullptr;
IPAddress cashAddress;
class UdpCashLink final : public parking::CashLink {
 public:
  void transmit(const parking::Message& request, bool discover) override {
    link.send(request, discover ? hardware::Datagram::discovery() : cashAddress);
  }
};
UdpCashLink cashLink;
hardware::ArduinoTicks uptime;
parking::GateStation* station = nullptr;
hardware::Gate barrierGate;
parking::BarrierOutput barrierOutput(barrierGate);
bool servosReady = false;

String statusText() {
  using parking::Notice;
  switch (controller->notice) {
    case Notice::Ready: return controller->ready() ? "Bereit. Einfahrt oder Ausfahrt auswaehlen." : "Kasse wird gesucht. Bitte beide Picos mit demselben WLAN verbinden.";
    case Notice::Checking: return "Anfrage wird an die Kasse gesendet. Bitte warten.";
    case Notice::EntryOpen: return controller->entryGate.isOpen() ? "Einfahrt registriert. Einfahrtsschranke offen." : "Einfahrt abgeschlossen. Jetzt an Pico 1 bezahlen.";
    case Notice::ExitOpen: return "Bezahlung bestaetigt. Ausfahrtsschranke offen.";
    case Notice::Denied: return ui::result(controller->result);
    case Notice::Timeout: return "Stoerung: Kasse antwortet nicht rechtzeitig. Keine neue Oeffnung. Vorgang wird abgeglichen; bitte Verbindung wiederherstellen.";
    case Notice::Reconciled: return "Verbindung wiederhergestellt. Verspaetete Freigabe wurde storniert. Vorgang bitte neu starten.";
    case Notice::Departed: return "Ausfahrt abgeschlossen und von der Kasse bestaetigt. Gute Fahrt!";
    case Notice::Restart: return "Kasse neu gestartet oder Sitzung gewechselt. Keine neue Freigabe. Gespeicherte Parkvorgaenge an der Kasse pruefen.";
    case Notice::Fault: return "Stoerung beim Zustandsabgleich. Keine neue Freigabe. Aufbau kontrollieren und beide Picos fuer einen neuen Test neu starten.";
  }
  return "Stoerung.";
}
String home(const String& error = "", bool refresh = false) {
  String p = ui::page("Pico 2 · Einfahrt und Ausfahrt", refresh);
  p += "<section><h2>Status</h2>";
  if (error.length()) p += ui::notice(error, true);
  p += ui::notice(statusText(), controller->notice == parking::Notice::Denied || controller->notice == parking::Notice::Timeout || controller->notice == parking::Notice::Fault);
  p += "<p>Gemeinsame Schranke: <strong>" + String(barrierGate.isOpen() ? "offen" : "geschlossen") + "</strong></p><p>Ein Servo fuer Einfahrt und Ausfahrt.</p><p>Kasse: ";
  p += controller->ready() ? "verbunden" : "nicht bereit";
  if (controller->serverBoot()) p += " · <a href='http://" + cashAddress.toString() + "/'>Kasse oeffnen</a>";
  p += "</p><p><a href='/'>Status aktualisieren</a></p></section>";
  if (!servosReady) p += ui::notice("Servo-Treiber nicht bereit. Es wird keine Freigabe angefordert.", true);
  p += "<section><h2>1. Einfahren</h2><p>Die manuelle Kennzeicheneingabe ersetzt die Kamera.</p><form method='post' action='/entry'><label>Kennzeichen <input name='plate' maxlength='16' placeholder='LU 123456' required></label><button>Einfahrt bestaetigen</button></form></section>";
  p += "<section><h2>3. Ausfahren</h2><p>Zuerst an Pico 1 bezahlen. Die Kasse muss jede Ausfahrt bestaetigen.</p><form method='post' action='/exit'><label>Kennzeichen <input name='plate' maxlength='16' placeholder='LU 123456' required></label><button>Ausfahrt pruefen</button></form></section><p>Die gemeinsame Schranke schliesst nach " + String(GATE_OPEN_MS) + " ms automatisch. Bewegungsbereich freihalten.</p>";
  if (PARKING_TEST_MODE) p += "<section><details><summary>Servotest ohne Parkvorgang</summary><p>Bewegungsbereich freihalten. Dieser Test steuert nur den Servo an GP8 an; er registriert keine Ein- oder Ausfahrt.</p><form method='post' action='/servo-test'>" + ui::pin() + "<button>Servo 5 Sekunden testen</button></form></details></section>";
  return p + ui::end();
}
hardware::Response handle(const parking::HttpRequest& q) {
  if (!strcmp(q.method, "GET") && !strcmp(q.path, "/api/status")) {
    const String message = statusText();
    String escaped;
    for (size_t i = 0; i < message.length(); ++i) {
      const char c = message[i];
      if (c == '"' || c == '\\') escaped += '\\';
      if (uint8_t(c) >= 0x20) escaped += c;
    }
    const String body = String("{\"busy\":") + (controller->busy() || barrierGate.isOpen() ? "true" : "false") +
      ",\"open\":" + (barrierGate.isOpen() ? "true" : "false") +
      ",\"ready\":" + (controller->ready() ? "true" : "false") +
      ",\"message\":\"" + escaped + "\"}";
    return {200, body, "application/json; charset=utf-8"};
  }
  if (!strcmp(q.method, "GET") && !strcmp(q.path, "/")) return {200, home("", controller->busy())};
  if (!strcmp(q.method, "POST") && !strcmp(q.path, "/servo-test") && PARKING_TEST_MODE) {
    if (!ui::admin(q)) return {403, home("Admin-PIN ungueltig.")};
    if (!servosReady) return {503, home("Servo-Treiber nicht bereit.")};
    if (controller->busy() || barrierGate.isOpen()) return {409, home("Vorgang oder Schranke noch aktiv. Bitte warten.")};
    barrierGate.open();
    Serial.printf("Servotest: GP%u auf %u Grad fuer %lu ms\n", BARRIER_SERVO_PIN, BARRIER_OPEN_ANGLE, (unsigned long)GATE_OPEN_MS);
    return {200, home("Servotest laeuft: GP8 offen. Nach 5 Sekunden wieder geschlossen.", true)};
  }
  if (strcmp(q.method, "POST") || (strcmp(q.path, "/entry") && strcmp(q.path, "/exit"))) return {404, home("Seite nicht gefunden.")};
  if (!servosReady) return {503, home("Servo-Treiber nicht bereit.")};
  if (barrierGate.isOpen()) return {409, home("Schranke noch aktiv. Bitte warten.")};
  char plate[parking::PlateSize];
  if (!ui::plate(q, plate)) return {400, home(ui::result(parking::Result::Invalid))};
  const auto op = !strcmp(q.path, "/entry") ? parking::Op::Entry : parking::Op::Authorize;
  auto r = station->action(op, plate);
  return {r == parking::Result::Ok ? 202 : 409, home(r == parking::Result::Ok ? "" : ui::result(r), r == parking::Result::Ok)};
}
hardware::HttpServer web(handle);
void serviceController() {
  station->poll();
  const bool open = controller->barrierOpen();
  if (barrierOutput.update(open)) {
    Serial.printf("Gemeinsame Schranke GP%u: %s\n", BARRIER_SERVO_PIN, open ? "offen" : "geschlossen");
  }
  barrierGate.update();
}
} // namespace

void setup() {
  Serial.begin(115200);
  barrierGate.begin();
  servosReady = barrierGate.attached();
  static parking::Controller gateController(hardware::bootId(), PARKING_LINK_ID, GATE_OPEN_MS); controller = &gateController;
  static parking::GateStation gateStation(gateController, uptime, cashLink);
  station = &gateStation;
  Serial.println("Pico 2: Eine gemeinsame Schranke an GP8, geschlossen.");
  network.begin("parkhaus-schranke");
}
void loop() {
  serviceController();
  const bool up = network.tick(); link.online(up); web.online(up);
  for (int budget = 0; budget < 4; ++budget) {
    parking::Message a; IPAddress from;
    if (!link.receive(a, from)) break;
    if (a.op != parking::Op::Hello && from != cashAddress && !controller->reconciling()) continue;
    if (station->receive(a)) {
      cashAddress = from; // Only after complete transaction/session correlation succeeded.
      if (a.op != parking::Op::Hello && a.op != parking::Op::Ping)
        Serial.printf("RPC reply seq=%lu op=%u plate=%s result=%u visit=%lu\n", (unsigned long)a.seq, unsigned(a.op), a.plate, unsigned(a.result), (unsigned long)a.visit);
    }
  }
  serviceController();
  web.tick();
  serviceController();
}
