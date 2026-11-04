#pragma once

// Netzwerkzugangsdaten liegen in einer lokalen, von Git ignorierten Datei.
// Ohne diese Datei bleibt das Projekt trotzdem kompilierbar.
#if __has_include("parkhaus_secrets.h")
#include "parkhaus_secrets.h"
#else
static constexpr char PARKING_WIFI_SSID[] = "CHANGE_ME_WIFI_SSID";
static constexpr char PARKING_WIFI_PASSWORD[] = "CHANGE_ME_WIFI_PASSWORD";
#endif

// Pico 1 meldet sich per mDNS als parkhaus-kasse.local.
static constexpr char PARKING_CASH_HOST[] = "parkhaus-kasse.local";

// Demo-Tarif: CHF 0.50 je angefangene 30 Minuten, hoechstens CHF 15.00
// je zusammenhaengendem 24-Stunden-Zeitraum. Pico 1 kann diese Werte
// waehrend des Betriebs ueber seine Kassen-Weboberflaeche aendern.
static constexpr uint16_t DEFAULT_TARIFF_INTERVAL_MINUTES = 30;
static constexpr uint32_t DEFAULT_TARIFF_UNIT_CENTS = 50;
static constexpr uint32_t DEFAULT_DAILY_MAX_CENTS = 1500;

// Maximal gleichzeitig gespeicherte Fahrzeuge. Keine Parkhaus-Kapazitaet;
// dies ist nur die feste Groesse des RAM-Speichers im Prototyp.
static constexpr uint8_t PARKING_RECORD_CAPACITY = 32;

