#pragma once
#include <Arduino.h>
#include "parking/core.h"
#include "parking/http_parser.h"
#include "parking/http_input.h"
#include "parkhaus_config.h"

namespace ui {
inline String number(uint64_t n) { char b[24]; snprintf(b, sizeof(b), "%llu", static_cast<unsigned long long>(n)); return String(b); }
inline String money(uint64_t cents) { return "CHF " + number(cents / 100) + "." + (cents % 100 < 10 ? "0" : "") + number(cents % 100); }
inline String duration(uint64_t ms) { return number(ms / 3600000) + " h " + number(ms / 60000 % 60) + " min " + number(ms / 1000 % 60) + " s"; }
inline const char* result(parking::Result r) {
  using parking::Result;
  switch (r) {
    case Result::Ok: return "Vorgang erfolgreich.";
    case Result::Invalid: return "Ungueltige Eingabe. Kennzeichen: 2-3 Buchstaben und 1-6 Ziffern, z. B. LU 123456.";
    case Result::Duplicate: return "Kennzeichen ist bereits eingefahren. Schranke bleibt geschlossen.";
    case Result::Full: return "Besetzt: Parkhaus ist voll. Einfahrt bleibt geschlossen.";
    case Result::NotFound: return "Kennzeichen nicht gefunden. Zuerst einfahren.";
    case Result::Unpaid: return "Unbezahlt. Bitte zuerst an der Kasse bezahlen. Ausfahrt gesperrt.";
    case Result::Expired: return "Kulanzzeit abgelaufen. Nachzahlung an der Kasse erforderlich.";
    case Result::Busy: return "Vorgang oder Schranke noch aktiv. Bitte warten.";
    case Result::Stale: return "Vorgang abgelaufen oder bereits verwendet. Gebuehr neu berechnen.";
    case Result::Session: return "Stoerung: Kasse nicht verbunden oder neu gestartet. Keine Freigabe.";
    case Result::Conflict: return "Zustand widerspruechlich. Aufbau pruefen und Testsystem kontrolliert zuruecksetzen.";
  }
  return "Stoerung.";
}
inline String page(const char* title, bool refresh = false) {
  String p = "<!doctype html><html lang='de'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>";
  p += title;
  p += "</title>";
  if (refresh) p += "<meta http-equiv='refresh' content='2;url=/'>";
  p += "<style>body{font:17px system-ui,sans-serif;background:#f0f4f8;color:#152536;margin:0}main{max-width:900px;margin:auto;padding:22px}header{background:#14365d;color:white;padding:22px;border-radius:12px}section{background:white;padding:20px;margin:18px 0;border-radius:12px}h1,h2{margin-top:0}label{display:block;margin:12px 0 6px}input,select,button{font:inherit;padding:10px;border:1px solid #9aaebe;border-radius:6px;max-width:100%;box-sizing:border-box}button{background:#175f94;color:white;cursor:pointer;margin:12px 8px 0 0}a{color:#175f94}.notice{padding:14px;background:#e3f1ff;border-left:5px solid #175f94}.error{background:#fff0e8;border-color:#ab3518}table{width:100%;border-collapse:collapse}td,th{padding:8px;text-align:left;border-bottom:1px solid #ddd}.scroll{overflow-x:auto}small{color:#43556b}</style></head><body><main><header><h1>";
  p += title; p += "</h1><div>Parkhaus-Prototyp · Kilian Schoeberl &amp; Noel Merz</div></header>";
  return p;
}
inline String end() { return "</main></body></html>"; }
inline String notice(const String& text, bool error = false) { return String("<p role='status' class='notice") + (error ? " error" : "") + "'>" + text + "</p>"; }
inline String hidden(const char* name, const String& value) { return String("<input type='hidden' name='") + name + "' value='" + value + "'>"; }
inline String pin() { return "<label>Admin-PIN <input name='pin' type='password' required autocomplete='off'></label>"; }
inline String numeric(const char* label, const char* name, uint32_t val, uint32_t min, uint32_t max) {
  return String("<label>") + label + " <input type='number' name='" + name + "' value='" + String(val) + "' min='" + String(min) + "' max='" + String(max) + "' required></label>";
}
inline bool admin(const parking::HttpRequest& q) { char p[64]; return q.arg("pin", p, sizeof(p)) && !strcmp(p, PARKING_ADMIN_PIN); }
inline bool integer(const parking::HttpRequest& q, const char* name, uint32_t min, uint32_t max, uint32_t& out) {
  char b[16]; return q.arg(name, b, sizeof(b)) && parking::unsignedNumber(b, min, max, out);
}
inline bool plate(const parking::HttpRequest& q, char* out) { return parking::HttpPlateInput(q).read(out, parking::PlateSize); }
} // namespace ui
