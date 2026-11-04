#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LEAmDNS.h>

#include "parkhaus_config.h"
#include "parking_common.h"

namespace {

WebServer server(80);

struct ParkingRecord {
  bool used = false;
  bool paid = false;
  bool entryTimeSimulated = false;
  char plate[16] = {};
  char paymentMethod[12] = {};
  uint32_t enteredAtMs = 0;
  uint32_t paidAtMs = 0;
  uint32_t paidAmountCents = 0;
};

ParkingRecord records[PARKING_RECORD_CAPACITY];
uint16_t tariffIntervalMinutes = DEFAULT_TARIFF_INTERVAL_MINUTES;
uint32_t tariffUnitCents = DEFAULT_TARIFF_UNIT_CENTS;
uint32_t dailyMaximumCents = DEFAULT_DAILY_MAX_CENTS;
bool mdnsStarted = false;
uint32_t lastWifiAttemptMs = 0;

ParkingRecord *findRecord(const char *plate) {
  for (ParkingRecord &record : records) {
    if (record.used && strcmp(record.plate, plate) == 0) {
      return &record;
    }
  }
  return nullptr;
}

ParkingRecord *findFreeRecord() {
  for (ParkingRecord &record : records) {
    if (!record.used) {
      return &record;
    }
  }
  return nullptr;
}

uint32_t calculateFeeCents(uint32_t durationMs) {
  const uint64_t intervalMs =
      static_cast<uint64_t>(tariffIntervalMinutes) * 60ULL * 1000ULL;
  const uint64_t dayMs = 24ULL * 60ULL * 60ULL * 1000ULL;
  const uint64_t fullDays = durationMs / dayMs;
  const uint64_t remainderMs = durationMs % dayMs;
  const uint64_t billedUnits =
      (remainderMs == 0 && fullDays > 0)
          ? 0
          : max(static_cast<uint64_t>(1), (remainderMs + intervalMs - 1) / intervalMs);
  const uint64_t partialDayCents =
      min(static_cast<uint64_t>(dailyMaximumCents),
          billedUnits * static_cast<uint64_t>(tariffUnitCents));
  const uint64_t totalCents =
      fullDays * static_cast<uint64_t>(dailyMaximumCents) + partialDayCents;
  return totalCents > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(totalCents);
}

String formatMoney(uint32_t cents) {
  char text[32];
  snprintf(text, sizeof(text), "CHF %lu.%02lu",
           static_cast<unsigned long>(cents / 100UL),
           static_cast<unsigned long>(cents % 100UL));
  return String(text);
}

String pageStart(const char *title) {
  String page = F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>");
  page += title;
  page += F("</title><style>"
            "body{font:16px Arial,sans-serif;margin:0;background:#f3f6fa;color:#172033}"
            "main{max-width:850px;margin:auto;padding:22px}"
            "header{background:#14365d;color:white;padding:20px;border-radius:12px}"
            "section{background:white;padding:18px;margin:16px 0;border-radius:12px;"
            "box-shadow:0 2px 8px #17203318}h1{margin:0 0 8px}h2{margin-top:0}"
            "label{display:block;margin:10px 0 5px;font-weight:bold}"
            "input,select,button{box-sizing:border-box;font:inherit;padding:10px;"
            "border:1px solid #b8c5d3;border-radius:7px}input,select{width:100%;"
            "max-width:420px}button{background:#1769aa;color:white;border:0;"
            "font-weight:bold;cursor:pointer;margin-top:12px}button:hover{background:#105488}"
            ".notice{padding:12px;background:#e7f2ff;border-left:4px solid #1769aa;"
            "border-radius:5px}.error{background:#fff0eb;border-color:#ba3d16}"
            "table{width:100%;border-collapse:collapse}td,th{text-align:left;"
            "padding:8px;border-bottom:1px solid #e4e9ef}small,.muted{color:#536276}"
            "code{background:#edf1f5;padding:2px 5px;border-radius:4px}"
            "</style></head><body><main><header><h1>Parkhaus-Kasse</h1>"
            "<div>Pico 1 · Gebuehr und Bezahlung</div></header>");
  return page;
}

void sendPage(String page) {
  page += F("</main></body></html>");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", page);
}

void showMessage(const String &message, bool isError = false) {
  String page = pageStart("Parkhaus-Kasse");
  page += F("<section><p class='notice");
  if (isError) {
    page += F(" error");
  }
  page += F("'>");
  page += message;
  page += F("</p><p><a href='/'>Zurueck zur Kasse</a></p></section>");
  sendPage(page);
}

void renderHome(const String &message = "") {
  String page = pageStart("Parkhaus-Kasse");
  page += F("<section><h2>Parkgebuehr berechnen</h2>"
            "<p>Ein Kennzeichen eingeben, um Parkdauer und offenen Betrag zu sehen.</p>"
            "<form method='post' action='/quote'><label for='plate'>Kennzeichen</label>"
            "<input id='plate' name='plate' placeholder='LU 123456' maxlength='16' required>"
            "<button type='submit'>Gebuehr berechnen</button></form></section>");

  if (message.length()) {
    page += F("<section><p class='notice'>");
    page += message;
    page += F("</p></section>");
  }

  page += F("<section><h2>Tarif einstellen</h2><p>Die Aenderung gilt sofort und bleibt bis "
            "zum Neustart dieses Pico gespeichert.</p><form method='post' action='/tariff'>"
            "<label for='interval'>Preisintervall in Minuten</label>"
            "<input id='interval' name='interval' type='number' min='1' max='1440' value='");
  page += String(tariffIntervalMinutes);
  page += F("' required><label for='price'>Preis je angefangenem Intervall (Rappen)</label>"
            "<input id='price' name='price' type='number' min='1' max='100000' value='");
  page += String(tariffUnitCents);
  page += F("' required><label for='maximum'>Maximum je 24 Stunden (Rappen)</label>"
            "<input id='maximum' name='maximum' type='number' min='1' max='10000000' value='");
  page += String(dailyMaximumCents);
  page += F("' required><button type='submit'>Tarif speichern</button></form><p class='muted'>"
            "Aktuell: ");
  page += formatMoney(tariffUnitCents);
  page += F(" je ");
  page += String(tariffIntervalMinutes);
  page += F(" Minuten, maximal ");
  page += formatMoney(dailyMaximumCents);
  page += F(" pro 24-Stunden-Abschnitt.</p></section>");

  page += F("<section><h2>Aktive Parkvorgaenge</h2><table><thead><tr><th>Kennzeichen</th>"
            "<th>Einfahrt</th><th>Dauer</th><th>Status</th></tr></thead><tbody>");
  bool anyRecord = false;
  for (const ParkingRecord &record : records) {
    if (!record.used) {
      continue;
    }
    anyRecord = true;
    const uint32_t duration = millis() - record.enteredAtMs;
    page += F("<tr><td>");
    page += record.plate;
    page += F("</td><td>");
    if (record.entryTimeSimulated) {
      page += F("simuliert");
    } else {
      page += formatUptime(record.enteredAtMs);
      page += F(" seit Start");
    }
    page += F("</td><td>");
    page += formatDuration(duration);
    page += F("</td><td>");
    page += record.paid ? F("bezahlt") : F("unbezahlt");
    page += F("</td></tr>");
  }
  if (!anyRecord) {
    page += F("<tr><td colspan='4' class='muted'>Keine Fahrzeuge erfasst.</td></tr>");
  }
  page += F("</tbody></table><p class='muted'>Zeitstempel sind Laufzeit seit Pico-Start. "
            "Ohne RTC/NTP steht keine Kalenderzeit zur Verfuegung.</p></section>");

  page += F("<section><h2>Netzwerk</h2><p>Kassenseite: <code>http://");
  page += WiFi.localIP().toString();
  page += F("/</code> · Pico-2-Schnittstelle: <code>/api/exit/authorize</code></p></section>");
  sendPage(page);
}

void renderQuote(ParkingRecord &record, const String &message = "") {
  const uint32_t duration = millis() - record.enteredAtMs;
  const uint32_t currentFee = calculateFeeCents(duration);
  String page = pageStart("Parkgebuehr");
  page += F("<section><h2>Fahrzeug ");
  page += record.plate;
  page += F("</h2><table><tr><th>Einfahrt</th><td>");
  if (record.entryTimeSimulated) {
    page += F("simuliert");
  } else {
    page += formatUptime(record.enteredAtMs);
    page += F(" seit Pico-Start");
  }
  page += F("</td></tr><tr><th>Parkdauer</th><td>");
  page += formatDuration(duration);
  page += F("</td></tr><tr><th>Gebuehr aktuell</th><td><strong>");
  page += formatMoney(record.paid ? record.paidAmountCents : currentFee);
  page += F("</strong></td></tr></table>");

  if (message.length()) {
    page += F("<p class='notice'>");
    page += message;
    page += F("</p>");
  }

  if (record.paid) {
    page += F("<p class='notice'>Bezahlt mit ");
    page += record.paymentMethod;
    page += F(" um ");
    page += formatUptime(record.paidAtMs);
    page += F(" seit Pico-Start.</p>");
  } else {
    page += F("<h3>Zahlung simulieren</h3><form method='post' action='/pay'>"
              "<input type='hidden' name='plate' value='");
    page += record.plate;
    page += F("'><label for='method'>Zahlungsmethode</label><select id='method' name='method'>"
              "<option value='Karte'>Karte (simuliert)</option>"
              "<option value='TWINT'>TWINT (simuliert)</option></select>"
              "<button type='submit'>Zahlung bestaetigen</button></form>"
              "<p class='muted'>Ohne Bestaetigung bleibt der Bezahlstatus unveraendert.</p>");

    page += F("<hr><h3>Testhilfe: Parkdauer simulieren</h3>"
              "<p class='muted'>Nur fuer TC-06/TC-07. Setzt fuer diesen offenen Vorgang "
              "eine Testdauer, ohne auf die Uhr warten zu muessen.</p>"
              "<form method='post' action='/simulate'><input type='hidden' name='plate' value='");
    page += record.plate;
    page += F("'><label for='minutes'>Vergangene Minuten</label><input id='minutes' "
              "name='minutes' type='number' min='0' max='43200' value='1' required>"
              "<button type='submit'>Testdauer setzen</button></form>");
  }
  page += F("<p><a href='/'>Zurueck zur Kasse</a></p></section>");
  sendPage(page);
}

void handleRoot() { renderHome(); }

void handleQuote() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    showMessage("Bitte ein gueltiges Kennzeichen wie LU 123456 eingeben.", true);
    return;
  }
  ParkingRecord *record = findRecord(plate);
  if (record == nullptr) {
    showMessage("Kennzeichen nicht gefunden. Zuerst muss die Einfahrt registriert werden.", true);
    return;
  }
  renderQuote(*record);
}

void handlePayment() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    showMessage("Bitte ein gueltiges Kennzeichen eingeben.", true);
    return;
  }
  ParkingRecord *record = findRecord(plate);
  if (record == nullptr) {
    showMessage("Kennzeichen nicht gefunden. Es wurde nichts bezahlt.", true);
    return;
  }
  if (record->paid) {
    renderQuote(*record, "Dieses Kennzeichen ist bereits bezahlt.");
    return;
  }

  const String method = server.arg("method");
  if (method != "Karte" && method != "TWINT") {
    renderQuote(*record, "Bitte Karte oder TWINT auswaehlen.");
    return;
  }

  record->paid = true;
  record->paidAtMs = millis();
  record->paidAmountCents = calculateFeeCents(record->paidAtMs - record->enteredAtMs);
  strncpy(record->paymentMethod, method.c_str(), sizeof(record->paymentMethod) - 1);
  record->paymentMethod[sizeof(record->paymentMethod) - 1] = '\0';
  Serial.printf("Zahlung: %s, %lu Rappen, Methode %s, Zeit %s\n",
                record->plate, static_cast<unsigned long>(record->paidAmountCents),
                record->paymentMethod, formatUptime(record->paidAtMs).c_str());
  renderQuote(*record, "Zahlung simuliert und mit Zeitstempel gespeichert.");
}

void handleTariffUpdate() {
  const long interval = server.arg("interval").toInt();
  const long price = server.arg("price").toInt();
  const long maximum = server.arg("maximum").toInt();
  if (interval < 1 || interval > 1440 || price < 1 || price > 100000 ||
      maximum < price || maximum > 10000000) {
    renderHome("Tarif ungueltig: Intervall, Preis und Tagesmaximum pruefen.");
    return;
  }
  tariffIntervalMinutes = static_cast<uint16_t>(interval);
  tariffUnitCents = static_cast<uint32_t>(price);
  dailyMaximumCents = static_cast<uint32_t>(maximum);
  Serial.printf("Tarif geaendert: %u min, %lu Rappen je Einheit, Maximum %lu Rappen/24h\n",
                tariffIntervalMinutes, static_cast<unsigned long>(tariffUnitCents),
                static_cast<unsigned long>(dailyMaximumCents));
  renderHome("Tarif wurde uebernommen. Neue Berechnungen verwenden die aktualisierten Werte.");
}

void handleTimeSimulation() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    showMessage("Bitte ein gueltiges Kennzeichen eingeben.", true);
    return;
  }
  ParkingRecord *record = findRecord(plate);
  if (record == nullptr) {
    showMessage("Kennzeichen nicht gefunden.", true);
    return;
  }
  if (record->paid) {
    renderQuote(*record, "Eine bezahlte Parkdauer kann nicht mehr simuliert werden.");
    return;
  }
  const long minutes = server.arg("minutes").toInt();
  if (minutes < 0 || minutes > 43200) {
    renderQuote(*record, "Bitte eine Testdauer von 0 bis 43200 Minuten eingeben.");
    return;
  }

  record->enteredAtMs = millis() - static_cast<uint32_t>(minutes) * 60000UL;
  record->entryTimeSimulated = true;
  Serial.printf("Testdauer gesetzt: %s, %ld Minuten\n", record->plate, minutes);
  renderQuote(*record, String("Testdauer auf ") + minutes + " Minuten gesetzt.");
}

void handleEntryRegistration() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    server.send(400, "text/plain", "INVALID_PLATE");
    return;
  }
  if (findRecord(plate) != nullptr) {
    Serial.printf("Einfahrt abgelehnt (Duplikat): %s\n", plate);
    server.send(409, "text/plain", "DUPLICATE");
    return;
  }
  ParkingRecord *record = findFreeRecord();
  if (record == nullptr) {
    server.send(503, "text/plain", "RECORD_STORE_FULL");
    return;
  }

  *record = ParkingRecord{};
  record->used = true;
  record->enteredAtMs = millis();
  strncpy(record->plate, plate, sizeof(record->plate) - 1);
  Serial.printf("Einfahrt registriert: %s, Zeit %s\n", record->plate,
                formatUptime(record->enteredAtMs).c_str());
  server.send(201, "text/plain", "ACCEPTED");
}

void handleExitAuthorization() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    server.send(400, "text/plain", "INVALID_PLATE");
    return;
  }
  ParkingRecord *record = findRecord(plate);
  if (record == nullptr) {
    Serial.printf("Ausfahrt abgelehnt (unbekannt): %s\n", plate);
    server.send(404, "text/plain", "NOT_FOUND");
    return;
  }
  if (!record->paid) {
    Serial.printf("Ausfahrt abgelehnt (unbezahlt): %s\n", plate);
    server.send(403, "text/plain", "UNPAID");
    return;
  }
  Serial.printf("Bezahlstatus bestaetigt: %s, bezahlt %s\n", plate,
                formatUptime(record->paidAtMs).c_str());
  server.send(200, "text/plain", "PAID");
}

void handleDeparted() {
  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    server.send(400, "text/plain", "INVALID_PLATE");
    return;
  }
  ParkingRecord *record = findRecord(plate);
  if (record == nullptr) {
    // Idempotent response: Pico 2 may retry if it lost the first reply.
    server.send(200, "text/plain", "DEPARTED");
    return;
  }
  if (!record->paid) {
    server.send(403, "text/plain", "UNPAID");
    return;
  }
  Serial.printf("Ausfahrt verbucht: %s\n", plate);
  *record = ParkingRecord{};
  server.send(200, "text/plain", "DEPARTED");
}

void configureServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/quote", HTTP_POST, handleQuote);
  server.on("/pay", HTTP_POST, handlePayment);
  server.on("/tariff", HTTP_POST, handleTariffUpdate);
  server.on("/simulate", HTTP_POST, handleTimeSimulation);
  server.on("/api/entry", HTTP_POST, handleEntryRegistration);
  server.on("/api/exit/authorize", HTTP_POST, handleExitAuthorization);
  server.on("/api/departed", HTTP_POST, handleDeparted);
  server.onNotFound([]() { server.send(404, "text/plain", "NOT_FOUND"); });
  server.begin();
}

void maintainNetwork() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mdnsStarted) {
      mdnsStarted = MDNS.begin("parkhaus-kasse");
      if (mdnsStarted) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS bereit: http://parkhaus-kasse.local/");
      }
      Serial.print("Kasse erreichbar unter http://");
      Serial.println(WiFi.localIP());
    }
    MDNS.update();
    return;
  }

  if (millis() - lastWifiAttemptMs >= 10000UL) {
    lastWifiAttemptMs = millis();
    Serial.println("WLAN getrennt; Verbindungsversuch...");
    WiFi.disconnect();
    WiFi.begin(PARKING_WIFI_SSID, PARKING_WIFI_PASSWORD);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Pico 1 - Parkhaus-Kasse startet");
  WiFi.mode(WIFI_STA);
  WiFi.begin(PARKING_WIFI_SSID, PARKING_WIFI_PASSWORD);
  configureServer();
  lastWifiAttemptMs = millis();
}

void loop() {
  server.handleClient();
  maintainNetwork();
}

