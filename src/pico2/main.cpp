#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <LEAmDNS.h>
#include <Servo.h>

#include "parkhaus_config.h"
#include "parking_common.h"

namespace {

constexpr uint8_t ENTRY_SERVO_PIN = 8;
constexpr uint8_t EXIT_SERVO_PIN = 9;
constexpr uint8_t CLOSED_ANGLE = 0;
constexpr uint8_t OPEN_ANGLE = 90;
constexpr uint32_t GATE_OPEN_MS = 2500;
constexpr uint32_t DEPARTURE_RETRY_MS = 5000;

WebServer server(80);
Servo entryServo;
Servo exitServo;

struct GateState {
  Servo *servo;
  const char *name;
  bool isOpen;
  uint32_t closeAtMs;
};

GateState entryGate{&entryServo, "Einfahrt", false, 0};
GateState exitGate{&exitServo, "Ausfahrt", false, 0};
bool mdnsStarted = false;
uint32_t lastWifiAttemptMs = 0;
uint32_t lastDepartureAttemptMs = 0;
bool departurePending = false;
char pendingDeparturePlate[16] = {};
String lastMessage = "Bereit. Kennzeichen fuer Einfahrt oder Ausfahrt eingeben.";
bool lastMessageIsError = false;

struct Pico1Reply {
  int status;
  String body;
};

String formEncode(const char *value) {
  String encoded;
  for (size_t i = 0; value[i] != '\0'; ++i) {
    const char c = value[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      char escape[4];
      snprintf(escape, sizeof(escape), "%%%02X", static_cast<unsigned char>(c));
      encoded += escape;
    }
  }
  return encoded;
}

Pico1Reply postToCashier(const char *path, const char *plate) {
  if (WiFi.status() != WL_CONNECTED) {
    return {-1, "WIFI_DISCONNECTED"};
  }

  WiFiClient client;
  HTTPClient http;
  String url = "http://";
  url += PARKING_CASH_HOST;
  url += path;
  if (!http.begin(client, url)) {
    return {-1, "HTTP_CLIENT_ERROR"};
  }
  http.setTimeout(2500);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const String body = String("plate=") + formEncode(plate);
  const int status = http.POST(body);
  String response = status > 0 ? http.getString() : "HTTP_REQUEST_FAILED";
  response.trim();
  http.end();
  return {status, response};
}

String pageStart() {
  String page = F("<!doctype html><html lang='de'><head><meta charset='utf-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>Parkhaus-Schranken</title><style>"
                  "body{font:16px Arial,sans-serif;margin:0;background:#f3f6fa;color:#172033}"
                  "main{max-width:800px;margin:auto;padding:22px}"
                  "header{background:#14365d;color:white;padding:20px;border-radius:12px}"
                  "section{background:white;padding:18px;margin:16px 0;border-radius:12px;"
                  "box-shadow:0 2px 8px #17203318}h1{margin:0 0 8px}h2{margin-top:0}"
                  "label{display:block;margin:10px 0 5px;font-weight:bold}"
                  "input,button{box-sizing:border-box;font:inherit;padding:10px;"
                  "border:1px solid #b8c5d3;border-radius:7px}input{width:100%;"
                  "max-width:420px}button{background:#1769aa;color:white;border:0;"
                  "font-weight:bold;cursor:pointer;margin-top:12px}button:hover{background:#105488}"
                  ".notice{padding:12px;background:#e7f2ff;border-left:4px solid #1769aa;"
                  "border-radius:5px}.error{background:#fff0eb;border-color:#ba3d16}"
                  ".gates{display:flex;gap:12px;flex-wrap:wrap}.gate{padding:10px 14px;"
                  "border-radius:7px;background:#e7f2ff}.closed{background:#e5f5e9}"
                  "small,.muted{color:#536276}code{background:#edf1f5;padding:2px 5px;"
                  "border-radius:4px}</style></head><body><main>"
                  "<header><h1>Parkhaus-Schranken</h1>"
                  "<div>Pico 2 · Ein-/Ausfahrt und Kamera-Simulation</div></header>");
  return page;
}

void sendPage() {
  String page = pageStart();
  page += F("<section><h2>Einfahrt registrieren</h2><p>Kennzeichen manuell eingeben; "
            "bei bestaetigter Registrierung oeffnet die Einfahrtsschranke.</p>"
            "<form method='post' action='/entry'><label for='entry-plate'>Kennzeichen</label>"
            "<input id='entry-plate' name='plate' placeholder='LU 123456' maxlength='16' required>"
            "<button type='submit'>Einfahrt bestaetigen</button></form></section>"
            "<section><h2>Ausfahrt pruefen</h2><p>Die Kasse wird vor dem Oeffnen online "
            "nach dem Bezahlstatus gefragt.</p><form method='post' action='/exit'>"
            "<label for='exit-plate'>Kennzeichen</label><input id='exit-plate' name='plate' "
            "placeholder='LU 123456' maxlength='16' required>"
            "<button type='submit'>Ausfahrt pruefen</button></form></section>");

  page += F("<section><h2>Rueckmeldung</h2><p class='notice");
  if (lastMessageIsError) {
    page += F(" error");
  }
  page += F("'>");
  page += lastMessage;
  page += F("</p><div class='gates'><span class='gate ");
  page += entryGate.isOpen ? F("") : F("closed");
  page += F("'>Einfahrt: ");
  page += entryGate.isOpen ? F("offen") : F("geschlossen");
  page += F("</span><span class='gate ");
  page += exitGate.isOpen ? F("") : F("closed");
  page += F("'>Ausfahrt: ");
  page += exitGate.isOpen ? F("offen") : F("geschlossen");
  page += F("</span></div></section>");

  page += F("<section><h2>Verbindung</h2><p>WLAN: ");
  page += WiFi.status() == WL_CONNECTED ? F("verbunden") : F("getrennt");
  page += F("<br>Schranken-Pico: <code>http://");
  page += WiFi.localIP().toString();
  page += F("/</code><br>Kassen-Pico: <code>http://");
  page += PARKING_CASH_HOST;
  page += F("/</code></p><p class='muted'>Bei Netzwerkfehler bleibt die Ausfahrtsschranke "
            "geschlossen.</p></section>");
  page += F("</main></body></html>");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html; charset=utf-8", page);
}

void setMessage(const String &message, bool isError = false) {
  lastMessage = message;
  lastMessageIsError = isError;
}

bool anyGateOpen() { return entryGate.isOpen || exitGate.isOpen; }

void openGate(GateState &gate) {
  gate.servo->write(OPEN_ANGLE);
  gate.isOpen = true;
  gate.closeAtMs = millis() + GATE_OPEN_MS;
  Serial.printf("%s-Schranke geoeffnet.\n", gate.name);
}

void handleEntry() {
  if (anyGateOpen()) {
    setMessage("Eine Schranke ist gerade in Bewegung. Bitte kurz warten.", true);
    sendPage();
    return;
  }

  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    setMessage("Kennzeichen ungueltig. Erlaubt sind 2-3 Buchstaben und 1-6 Ziffern, z. B. LU 123456.", true);
    sendPage();
    return;
  }

  Serial.printf("Einfahrt an Kasse registrieren: %s\n", plate);
  const Pico1Reply reply = postToCashier("/api/entry", plate);
  if (reply.status == 201 && reply.body == "ACCEPTED") {
    setMessage(String("Einfahrt fuer ") + plate + " registriert. Schranke oeffnet.");
    openGate(entryGate);
  } else if (reply.status == 409 && reply.body == "DUPLICATE") {
    setMessage(String(plate) + " ist bereits als eingefahren erfasst.", true);
  } else if (reply.status == 400 && reply.body == "INVALID_PLATE") {
    setMessage("Kennzeichen wurde von der Kasse abgelehnt.", true);
  } else if (reply.status <= 0) {
    setMessage("Stoerung: Kasse nicht erreichbar. Einfahrt wurde nicht freigegeben.", true);
  } else {
    setMessage("Einfahrt konnte nicht registriert werden. Status: " + reply.body, true);
  }
  sendPage();
}

void handleExit() {
  if (anyGateOpen() || departurePending) {
    setMessage("Eine Schranke ist noch aktiv oder eine Ausfahrt wird verbucht. Bitte kurz warten.", true);
    sendPage();
    return;
  }

  char plate[16];
  if (!normalizePlate(server.arg("plate"), plate, sizeof(plate))) {
    setMessage("Kennzeichen ungueltig. Erlaubt sind 2-3 Buchstaben und 1-6 Ziffern, z. B. LU 123456.", true);
    sendPage();
    return;
  }

  Serial.printf("Bezahlstatus bei Pico 1 abfragen: %s\n", plate);
  const Pico1Reply reply = postToCashier("/api/exit/authorize", plate);
  Serial.printf("Antwort Pico 1: HTTP %d, %s\n", reply.status, reply.body.c_str());

  if (reply.status == 200 && reply.body == "PAID") {
    strncpy(pendingDeparturePlate, plate, sizeof(pendingDeparturePlate) - 1);
    pendingDeparturePlate[sizeof(pendingDeparturePlate) - 1] = '\0';
    departurePending = true;
    lastDepartureAttemptMs = 0;
    setMessage(String("Bezahlung fuer ") + plate + " bestaetigt. Ausfahrtsschranke oeffnet.");
    openGate(exitGate);
  } else if (reply.status == 403 && reply.body == "UNPAID") {
    setMessage(String(plate) + " ist noch nicht bezahlt. Ausfahrt bleibt gesperrt.", true);
  } else if (reply.status == 404 && reply.body == "NOT_FOUND") {
    setMessage(String(plate) + " ist nicht als eingefahren erfasst.", true);
  } else if (reply.status <= 0) {
    setMessage("Stoerung: Kasse nicht erreichbar. Ausfahrtsschranke bleibt geschlossen.", true);
  } else {
    setMessage("Ausfahrt nicht freigegeben. Antwort der Kasse: " + reply.body, true);
  }
  sendPage();
}

void configureServer() {
  server.on("/", HTTP_GET, sendPage);
  server.on("/entry", HTTP_POST, handleEntry);
  server.on("/exit", HTTP_POST, handleExit);
  server.onNotFound([]() {
    setMessage("Seite nicht gefunden.", true);
    sendPage();
  });
  server.begin();
}

void maintainNetwork() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mdnsStarted) {
      mdnsStarted = MDNS.begin("parkhaus-schranke");
      if (mdnsStarted) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("mDNS bereit: http://parkhaus-schranke.local/");
      }
      Serial.print("Schranken-Webseite erreichbar unter http://");
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

void serviceGates() {
  const uint32_t now = millis();
  GateState *gates[] = {&entryGate, &exitGate};
  for (GateState *gate : gates) {
    if (gate->isOpen && static_cast<int32_t>(now - gate->closeAtMs) >= 0) {
      gate->servo->write(CLOSED_ANGLE);
      gate->isOpen = false;
      Serial.printf("%s-Schranke geschlossen.\n", gate->name);
    }
  }

  if (departurePending && !exitGate.isOpen &&
      now - lastDepartureAttemptMs >= DEPARTURE_RETRY_MS) {
    lastDepartureAttemptMs = now;
    const Pico1Reply reply = postToCashier("/api/departed", pendingDeparturePlate);
    if ((reply.status == 200 && reply.body == "DEPARTED") || reply.status == 404) {
      Serial.printf("Ausfahrt bei Pico 1 abgeschlossen: %s\n", pendingDeparturePlate);
      departurePending = false;
      pendingDeparturePlate[0] = '\0';
    } else {
      Serial.printf("Ausfahrt noch nicht verbucht; Wiederholung folgt: HTTP %d, %s\n",
                    reply.status, reply.body.c_str());
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("Pico 2 - Parkhaus-Schranken startet");

  entryServo.attach(ENTRY_SERVO_PIN);
  exitServo.attach(EXIT_SERVO_PIN);
  entryServo.write(CLOSED_ANGLE);
  exitServo.write(CLOSED_ANGLE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(PARKING_WIFI_SSID, PARKING_WIFI_PASSWORD);
  configureServer();
  lastWifiAttemptMs = millis();
}

void loop() {
  server.handleClient();
  serviceGates();
  maintainNetwork();
}

