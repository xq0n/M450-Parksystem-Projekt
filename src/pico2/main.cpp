#include <Arduino.h>
#include <Servo.h>
#include "parking/controller.h"
#include "drivers/runtime.h"
#include "drivers/http_server.h"
#include "web/ui.h"

namespace {
hardware::Network network;
hardware::Datagram link;
parking::Controller* controller = nullptr;
IPAddress cashAddress;
Servo entryServo, exitServo;
bool servosReady = false, entryWasOpen = false, exitWasOpen = false;

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
  p += "<p>Einfahrt: <strong>" + String(controller->entryGate.isOpen() ? "offen" : "geschlossen") + "</strong> · Ausfahrt: <strong>" + String(controller->exitGate.isOpen() ? "offen" : "geschlossen") + "</strong></p><p>Kasse: ";
  p += controller->ready() ? "verbunden" : "nicht bereit";
  if (controller->serverBoot()) p += " · <a href='http://" + cashAddress.toString() + "/'>Kasse oeffnen</a>";
  p += "</p><p><a href='/'>Status aktualisieren</a></p></section>";
  if (!servosReady) p += ui::notice("Servo-Treiber nicht bereit. Es wird keine Freigabe angefordert.", true);
  p += "<section><h2>1. Einfahren</h2><p>Die manuelle Kennzeicheneingabe ersetzt die Kamera.</p><form method='post' action='/entry'><label>Kennzeichen <input name='plate' maxlength='16' placeholder='LU 123456' required></label><button>Einfahrt bestaetigen</button></form></section>";
  p += "<section><h2>3. Ausfahren</h2><p>Zuerst an Pico 1 bezahlen. Die Kasse muss jede Ausfahrt bestaetigen.</p><form method='post' action='/exit'><label>Kennzeichen <input name='plate' maxlength='16' placeholder='LU 123456' required></label><button>Ausfahrt pruefen</button></form></section><p>Die Modellschranken schliessen nach " + String(GATE_OPEN_MS) + " ms automatisch. Bewegungsbereich freihalten.</p>";
  return p + ui::end();
}
hardware::Response handle(const parking::HttpRequest& q) {
  if (!strcmp(q.method, "GET") && !strcmp(q.path, "/")) return {200, home("", controller->busy())};
  if (strcmp(q.method, "POST") || (strcmp(q.path, "/entry") && strcmp(q.path, "/exit"))) return {404, home("Seite nicht gefunden.")};
  if (!servosReady) return {503, home("Servo-Treiber nicht bereit.")};
  char plate[parking::PlateSize];
  if (!ui::plate(q, plate)) return {400, home(ui::result(parking::Result::Invalid))};
  const auto op = !strcmp(q.path, "/entry") ? parking::Op::Entry : parking::Op::Authorize;
  auto r = controller->action(op, plate, millis());
  return {r == parking::Result::Ok ? 202 : 409, home(r == parking::Result::Ok ? "" : ui::result(r), r == parking::Result::Ok)};
}
hardware::HttpServer web(handle);
void serviceController() {
  parking::Message q;
  if (controller->poll(millis(), q)) {
    // After timeout rediscover even an outstanding transaction (DHCP address may change).
    link.send(q, q.op == parking::Op::Hello || controller->reconciling() ? hardware::Datagram::discovery() : cashAddress);
  }
  const bool entry = controller->entryGate.isOpen(), exit = controller->exitGate.isOpen();
  if (entry != entryWasOpen) { entryServo.write(entry ? ENTRY_OPEN_ANGLE : ENTRY_CLOSED_ANGLE); entryWasOpen = entry; }
  if (exit != exitWasOpen) { exitServo.write(exit ? EXIT_OPEN_ANGLE : EXIT_CLOSED_ANGLE); exitWasOpen = exit; }
}
} // namespace

void setup() {
  Serial.begin(115200);
  entryServo.attach(ENTRY_SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US, ENTRY_CLOSED_ANGLE);
  exitServo.attach(EXIT_SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US, EXIT_CLOSED_ANGLE);
  entryServo.write(ENTRY_CLOSED_ANGLE); exitServo.write(EXIT_CLOSED_ANGLE);
  servosReady = entryServo.attached() && exitServo.attached();
  static parking::Controller gateController(hardware::bootId(), PARKING_LINK_ID, GATE_OPEN_MS); controller = &gateController;
  Serial.println("Pico 2: Schranken. Beide Schranken geschlossen.");
  network.begin("parkhaus-schranke");
}
void loop() {
  serviceController();
  const bool up = network.tick(); link.online(up); web.online(up);
  for (int budget = 0; budget < 4; ++budget) {
    parking::Message a; IPAddress from;
    if (!link.receive(a, from)) break;
    if (a.op != parking::Op::Hello && from != cashAddress && !controller->reconciling()) continue;
    if (controller->receive(a, millis())) {
      cashAddress = from; // Only after complete transaction/session correlation succeeded.
      if (a.op != parking::Op::Hello && a.op != parking::Op::Ping)
        Serial.printf("RPC reply seq=%lu op=%u plate=%s result=%u visit=%lu\n", (unsigned long)a.seq, unsigned(a.op), a.plate, unsigned(a.result), (unsigned long)a.visit);
    }
  }
  serviceController();
  web.tick();
  serviceController();
}
