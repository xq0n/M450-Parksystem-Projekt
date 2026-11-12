#pragma once

// Netzwerkzugangsdaten liegen in einer lokalen, von Git ignorierten Datei.
// Ohne diese Datei bleibt das Projekt trotzdem kompilierbar.
#if __has_include("parkhaus_secrets.h")
#include "parkhaus_secrets.h"
#else
static constexpr char PARKING_WIFI_SSID[] = "CHANGE_ME_WIFI_SSID";
static constexpr char PARKING_WIFI_PASSWORD[] = "CHANGE_ME_WIFI_PASSWORD";
#endif

#include <stdint.h>

// UDP discovery in the same LAN. Set a numeric address if broadcasts are blocked.
// Example only: "192.168.1.42". Empty means automatic discovery.
static constexpr char PARKING_CASH_IP[] = "";
static constexpr uint16_t PARKING_LINK_PORT = 4210;
static constexpr uint32_t PARKING_LINK_ID = 0x4502026;
// All administrative/test mutations require this local prototype PIN.
// Override in parkhaus_secrets.h. This is not Internet-grade authentication.
#ifndef PARKING_ADMIN_PIN
#define PARKING_ADMIN_PIN "4500"
#endif
static constexpr bool PARKING_TEST_MODE = true;

// Actual hardware: ONE shared entry/exit barrier on GP8 (PDF example).
static constexpr uint8_t BARRIER_SERVO_PIN = 8;
static constexpr uint8_t BARRIER_CLOSED_ANGLE = 0, BARRIER_OPEN_ANGLE = 90;
static constexpr uint32_t GATE_OPEN_MS = 5000;

// Demo-Tarif: CHF 0.50 je angefangene 30 Minuten, hoechstens CHF 15.00
// je zusammenhaengendem 24-Stunden-Zeitraum. Pico 1 kann diese Werte
// waehrend des Betriebs ueber seine Kassen-Weboberflaeche aendern.
static constexpr uint16_t DEFAULT_TARIFF_INTERVAL_MINUTES = 30;
static constexpr uint32_t DEFAULT_TARIFF_UNIT_CENTS = 50;
static constexpr uint32_t DEFAULT_DAILY_MAX_CENTS = 1500;

static constexpr uint16_t DEFAULT_GRACE_MINUTES = 15;
static constexpr uint8_t DEFAULT_PARKING_CAPACITY = 10;
static_assert(DEFAULT_PARKING_CAPACITY >= 1 && DEFAULT_PARKING_CAPACITY <= 32, "Capacity must be 1..32");
static_assert(BARRIER_CLOSED_ANGLE <= 180 && BARRIER_OPEN_ANGLE <= 180, "Servo angles must be 0..180");
static_assert(GATE_OPEN_MS > 0 && GATE_OPEN_MS < 60000, "Gate hold must be 1..59999 ms");

