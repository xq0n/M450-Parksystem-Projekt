#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "parkhaus_config.h"
#include "parking/protocol.h"

namespace hardware {
inline uint32_t bootId() { uint32_t n = rp2040.hwrand32(); return n ? n : 1; }
class Network {
 public:
  void begin(const char* hostname) {
    WiFi.mode(WIFI_STA); WiFi.setHostname(hostname);
    WiFi.beginNoBlock(PARKING_WIFI_SSID, PARKING_WIFI_PASSWORD); last_ = millis();
  }
  bool tick() {
    const bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && (!up_ || uint32_t(millis() - report_) >= 15000)) {
      report_ = millis(); Serial.print("Weboberflaeche: http://"); Serial.println(WiFi.localIP());
    }
    if (!connected && uint32_t(millis() - last_) >= 15000) {
      last_ = millis(); Serial.println("WLAN: neuer Verbindungsversuch");
      WiFi.disconnect(); WiFi.beginNoBlock(PARKING_WIFI_SSID, PARKING_WIFI_PASSWORD);
    }
    up_ = connected; return connected;
  }
 private:
  bool up_ = false;
  uint32_t last_ = 0, report_ = 0;
};
class Datagram {
 public:
  void online(bool up) {
    if (!up && listening_) { udp_.stop(); listening_ = false; }
    if (up && !listening_) listening_ = udp_.begin(PARKING_LINK_PORT);
  }
  bool receive(parking::Message& m, IPAddress& from) {
    if (!listening_) return false;
    const int size = udp_.parsePacket(); if (!size) return false;
    from = udp_.remoteIP(); const uint16_t port = udp_.remotePort();
    uint8_t data[parking::FrameSize]; const int n = udp_.read(data, sizeof(data));
    // parsePacket discards remaining bytes of the previous datagram on the next call.
    if (size != int(sizeof(data)) || n != size || port != PARKING_LINK_PORT) return false;
    return parking::decode(data, n, m) && m.link == PARKING_LINK_ID;
  }
  void send(const parking::Message& m, IPAddress to) {
    if (!listening_) return;
    uint8_t data[parking::FrameSize]; parking::encode(m, data);
    if (udp_.beginPacket(to, PARKING_LINK_PORT)) { udp_.write(data, sizeof(data)); udp_.endPacket(); }
  }
  static IPAddress discovery() {
    IPAddress configured;
    if (configured.fromString(PARKING_CASH_IP)) return configured;
    IPAddress ip = WiFi.localIP(), mask = WiFi.subnetMask();
    return IPAddress(ip[0] | uint8_t(~mask[0]), ip[1] | uint8_t(~mask[1]), ip[2] | uint8_t(~mask[2]), ip[3] | uint8_t(~mask[3]));
  }
 private:
  WiFiUDP udp_;
  bool listening_ = false;
};
} // namespace hardware
