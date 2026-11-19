#include <Arduino.h>
#include "parking/core.h"
#include "parking/protocol.h"
#include "drivers/runtime.h"
#include "drivers/http_server.h"
#include "web/ui.h"

namespace {
parking::Store store;
parking::Clock clockSource;
hardware::ArduinoTicks uptime;
parking::Service* service = nullptr;
hardware::Network network;
hardware::Datagram link;
IPAddress gateAddress;

String home() {
  using namespace ui;
  String p = page("Pico 1 · Parkhaus-Kasse");
  p += "<section><h2>2. Parkgebuehr berechnen und bezahlen</h2><p>Zuerst an der Schranken-Seite einfahren. Danach hier das Kennzeichen eingeben.</p><form method='post' action='/quote'><label>Kennzeichen <input name='plate' placeholder='LU 123456' maxlength='16' required></label><button>Gebuehr berechnen</button></form></section>";
  p += "<section><h2>Aktive Parkvorgaenge</h2><p>Belegt: " + String(store.count()) + " / " + String(store.capacity) + "</p><div class='scroll'><table><tr><th>Kennzeichen</th><th>Einfahrt seit Start</th><th>Parkdauer</th><th>Status</th></tr>";
  const uint64_t now = clockSource.now();
  for (size_t i = 0; i < parking::MaxRecords; ++i) {
    const auto& r = store.records()[i]; if (!r.used) continue;
    p += "<tr><td>" + String(r.plate) + "</td><td>" + duration(r.entered) + "</td><td>" + duration(now - r.entered) + "</td><td>";
    p += r.exiting ? "Ausfahrt freigegeben" : !r.paid ? "unbezahlt" : now <= r.graceUntil ? "bezahlt" : "Nachzahlung erforderlich";
    p += "</td></tr>";
  }
  p += "</table></div><p><small>Zeitstempel: monotone Laufzeit der Kasse; keine Kalenderzeit. Daten und Tarif liegen im RAM und gehen beim Neustart verloren.</small></p></section>";
  p += "<section><h2>Tarif und Kapazitaet</h2><p>" + money(store.tariff.unitCents) + " je angefangene " + String(store.tariff.intervalMinutes) + " Minuten; maximal " + money(store.tariff.dailyMaxCents) + " je 24 Stunden. Kulanz nach Zahlung: " + String(store.tariff.graceMinutes) + " Minuten.</p><details><summary>Einstellungen aendern</summary><form method='post' action='/tariff'>";
  p += numeric("Intervall (Minuten)", "interval", store.tariff.intervalMinutes, 1, 1440);
  p += numeric("Preis pro Intervall (Rappen)", "price", store.tariff.unitCents, 1, 100000);
  p += numeric("Maximum pro 24 Stunden (Rappen)", "maximum", store.tariff.dailyMaxCents, 1, 10000000);
  p += numeric("Kulanzzeit (Minuten)", "grace", store.tariff.graceMinutes, 1, 1440);
  p += numeric("Parkplaetze", "capacity", store.capacity, 1, parking::MaxRecords);
  p += pin() + "<button>Einstellungen speichern</button></form><p>Aenderungen gelten fuer neue Berechnungen. Bereits bestaetigte Zahlungen und deren Kulanzfrist bleiben bestehen.</p></details></section>";
  if (PARKING_TEST_MODE) {
    p += "<section><h2>Testmodus aktiv</h2><p>Kamera und Karte/TWINT werden simuliert. Die Servos werden echt bewegt.</p><details><summary>Testzeit vorsetzen (TC-06/07/15/16)</summary><p>Verschiebt die Uhr fuer alle Fahrzeuge, ohne die Schranken-Timer zu beschleunigen. Vorher offene Zahlungsangebote werden ungueltig.</p><form method='post' action='/advance'>";
    p += numeric("Zusaetzliche Minuten", "minutes", 1, 1, 43200) + pin() + "<button>Testzeit vorsetzen</button></form></details><p>Grundzustand: beide Picos ausschalten, Schranken freihalten, zuerst Kasse und danach Schranken-Pico neu starten.</p></section>";
  }
  p += "<p><a href='/'>Ansicht aktualisieren</a> · Kasse: http://" + WiFi.localIP().toString() + "/";
  if (gateAddress != IPAddress()) p += " · <a href='http://" + gateAddress.toString() + "/'>Schranken-Seite oeffnen</a>";
  p += "</p>";
  return p + end();
}

hardware::Response message(const String& text, int status = 200) {
  return {status, ui::page("Parkhaus-Kasse") + "<section>" + ui::notice(text, status != 200) + "<p><a href='/'>Zurueck zur Kasse</a></p></section>" + ui::end()};
}
hardware::Response result(parking::Result r) { return message(ui::result(r), r == parking::Result::Ok ? 200 : 409); }

hardware::Response quote(const char* plate) {
  uint32_t id = 0;
  auto status = store.quote(plate, clockSource.now(), id);
  if (status != parking::Result::Ok) return result(status);
  const auto& r = *store.find(plate);
  String p = ui::page("Parkgebuehr und Zahlung");
  p += "<section><h2>" + String(r.plate) + "</h2><p>Einfahrt: " + ui::duration(r.entered) + " seit Kassenstart<br>Parkdauer: " + ui::duration(clockSource.now() - r.entered) + "</p>";
  if (r.paid) {
    p += "<p>Bereits bezahlt: " + ui::money(r.paidCents) + "<br>Letzte Zahlung: " + ui::duration(r.paidAt) + " seit Start, Methode: " + (r.method == parking::Method::Card ? "Karte" : "TWINT") + "<br>Kulanz bis: " + ui::duration(r.graceUntil) + " seit Start.</p>";
  }
  if (!id) p += ui::notice("Bezahlt. Innerhalb der Kulanzzeit an der Schranken-Seite ausfahren.");
  else {
    p += "<h2>" + String(r.paid ? "Nachzahlung: " : "Zu bezahlen: ") + ui::money(r.quoteCents) + "</h2><p>Dieser Betrag ist 2 Minuten gueltig. Erst die Bestaetigung setzt den Status auf bezahlt.</p><form method='post' action='/pay'>";
    p += ui::hidden("plate", r.plate) + ui::hidden("quote", String(id));
    p += "<label>Zahlungsmethode <select name='method'><option value='card'>Karte (simuliert)</option><option value='twint'>TWINT (simuliert)</option></select></label><button>Zahlung verbindlich simulieren</button></form><form method='post' action='/cancel'>";
    p += ui::hidden("plate", r.plate) + ui::hidden("quote", String(id));
    p += "<button>Zahlung abbrechen</button></form>";
  }
  p += "<p><a href='/'>Zurueck zur Kasse</a></p></section>";
  return {200, p + ui::end()};
}

hardware::Response handle(const parking::HttpRequest& q) {
  clockSource.update(uptime.ticks());
  if (!strcmp(q.method, "GET") && !strcmp(q.path, "/api/config")) {
    String body = String("{\"gate\":\"");
    if (gateAddress != IPAddress()) body += "http://" + gateAddress.toString();
    body += "\"}";
    return {200, body, "application/json; charset=utf-8"};
  }
  if (!strcmp(q.method, "GET") && !strcmp(q.path, "/")) return {200, home()};
  if (strcmp(q.method, "POST")) return message("Seite nicht gefunden.", 404);
  if (!strcmp(q.path, "/tariff")) {
    if (!ui::admin(q)) return message("Admin-PIN ungueltig.", 403);
    parking::Tariff t; uint32_t cap;
    if (!ui::integer(q, "interval", 1, 1440, t.intervalMinutes) || !ui::integer(q, "price", 1, 100000, t.unitCents) ||
        !ui::integer(q, "maximum", 1, 10000000, t.dailyMaxCents) || !ui::integer(q, "grace", 1, 1440, t.graceMinutes) ||
        !ui::integer(q, "capacity", 1, parking::MaxRecords, cap)) return message("Ungueltiger Tarif oder Kapazitaet.", 400);
    auto r = store.configure(t, cap);
    if (r != parking::Result::Ok) return message("Maximum muss mindestens Intervallpreis sein; Kapazitaet darf nicht kleiner als aktuelle Belegung sein.", 400);
    return message("Tarif und Kapazitaet gespeichert. Gebuehr neu berechnen.");
  }
  if (!strcmp(q.path, "/advance")) {
    if (!PARKING_TEST_MODE) return message("Testmodus deaktiviert.", 403);
    if (!ui::admin(q)) return message("Admin-PIN ungueltig.", 403);
    uint32_t minutes;
    if (!ui::integer(q, "minutes", 1, 43200, minutes)) return message("Testzeit ungueltig.", 400);
    clockSource.advance(uint64_t(minutes) * 60000); store.invalidateQuotes();
    return message("Testuhr um " + String(minutes) + " Minuten vorgestellt. Jetzt Gebuehr oder Ausfahrt neu pruefen.");
  }
  if (strcmp(q.path, "/quote") && strcmp(q.path, "/pay") && strcmp(q.path, "/cancel")) return message("Seite nicht gefunden.", 404);
  char plate[parking::PlateSize];
  if (!ui::plate(q, plate)) return result(parking::Result::Invalid);
  if (!strcmp(q.path, "/quote")) return quote(plate);
  if (!store.find(plate)) return result(parking::Result::NotFound);
  uint32_t id;
  if (!ui::integer(q, "quote", 1, UINT32_MAX, id)) return result(parking::Result::Stale);
  if (!strcmp(q.path, "/cancel")) {
    auto r = store.cancelPayment(plate, id);
    return r == parking::Result::Ok ? message("Zahlung abgebrochen. Es wurde kein Ausfahrtsrecht erteilt.") : result(r);
  }
  char method[12];
  if (!q.arg("method", method, sizeof(method))) return result(parking::Result::Invalid);
  auto m = !strcmp(method, "card") ? parking::Method::Card : !strcmp(method, "twint") ? parking::Method::Twint : parking::Method::None;
  auto r = store.pay(plate, id, m, clockSource.now());
  if (r != parking::Result::Ok) return result(r);
  Serial.printf("PAY plate=%s quote=%lu method=%s\n", plate, (unsigned long)id, method);
  const auto& record = *store.find(plate);
  return message(String("Zahlung bestaetigt fuer ") + plate + ". Bezahlt: " + ui::money(record.paidCents) + ". Zahlungszeit: " + ui::duration(record.paidAt) + " seit Start. Ausfahrt bis " + ui::duration(record.graceUntil) + " seit Start moeglich.");
}
hardware::HttpServer web(handle);
} // namespace

void setup() {
  Serial.begin(115200);
  store.tariff.intervalMinutes = DEFAULT_TARIFF_INTERVAL_MINUTES;
  store.tariff.unitCents = DEFAULT_TARIFF_UNIT_CENTS;
  store.tariff.dailyMaxCents = DEFAULT_DAILY_MAX_CENTS;
  store.tariff.graceMinutes = DEFAULT_GRACE_MINUTES;
  store.capacity = DEFAULT_PARKING_CAPACITY;
  static parking::Service server(store, hardware::bootId(), PARKING_LINK_ID); service = &server;
  Serial.println("Pico 1: Kasse. RAM-Zustand leer.");
  network.begin("parkhaus-kasse");
}
void loop() {
  clockSource.update(uptime.ticks());
  const bool up = network.tick(); link.online(up); web.online(up);
  for (int budget = 0; budget < 4; ++budget) {
    parking::Message q, a; IPAddress from;
    if (!link.receive(q, from)) break;
    if (service->handle(q, clockSource.now(), a)) {
      if (q.op == parking::Op::Hello || q.op == parking::Op::Ping) gateAddress = from;
      link.send(a, from);
      if (q.op != parking::Op::Hello && q.op != parking::Op::Ping)
        Serial.printf("RPC seq=%lu op=%u plate=%s result=%u visit=%lu\n", (unsigned long)q.seq, unsigned(q.op), q.plate, unsigned(a.result), (unsigned long)a.visit);
    }
  }
  web.tick();
}
