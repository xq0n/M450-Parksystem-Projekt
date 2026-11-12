#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include "parking/http_parser.h"

namespace hardware {
struct Response { int status = 200; String body; String contentType = "text/html; charset=utf-8"; };
using Handler = Response (*)(const parking::HttpRequest&);
// Bounded reads/writes. No readStringUntil(), connection wait, DNS or delay loop.
class HttpServer {
 public:
  explicit HttpServer(Handler handler) : server_(80), handler_(handler) {}
  void online(bool up) {
    if (!up && listening_) { close(); server_.end(); listening_ = false; }
    if (up && !listening_) { server_.begin(80, 2); listening_ = true; }
  }
  void tick() {
    if (!listening_) return;
    if (!client_) {
      client_ = server_.accept(); if (!client_) return;
      client_.setNoDelay(true); client_.setSync(false); client_.setTimeout(1);
      request_ = parking::HttpRequest{}; output_ = ""; sent_ = 0; started_ = millis();
    }
    if (uint32_t(millis() - started_) > 3000 || (!client_.connected() && !client_.available())) { close(); return; }
    if (!output_.length()) {
      for (int budget = 0; budget < 256 && client_.available() && request_.state == parking::HttpRequest::Reading; ++budget)
        request_.feed(char(client_.read()));
      if (request_.state == parking::HttpRequest::Reading) return;
      Response r;
      if (request_.state == parking::HttpRequest::Bad) { r.status = 400; r.body = "Ungueltige oder zu grosse Anfrage."; }
      else r = handler_(request_);
      if (r.body.length() > 24000) { r.status = 500; r.body = "Antwort zu gross."; }
      output_ = "HTTP/1.1 " + String(r.status) + " Response\r\nContent-Type: " + r.contentType + "\r\nCache-Control: no-store\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: " + String(r.body.length()) + "\r\n\r\n" + r.body;
      started_ = millis();
    }
    int room = client_.availableForWrite();
    if (room > 0) {
      size_t n = output_.length() - sent_; if (n > 512) n = 512; if (n > size_t(room)) n = room;
      sent_ += client_.write(reinterpret_cast<const uint8_t*>(output_.c_str()) + sent_, n);
    }
    if (sent_ == output_.length()) close();
  }
 private:
  void close() { client_.stop(1); client_ = WiFiClient(); output_ = ""; }
  WiFiServer server_;
  WiFiClient client_;
  Handler handler_;
  parking::HttpRequest request_;
  String output_;
  size_t sent_ = 0;
  uint32_t started_ = 0;
  bool listening_ = false;
};
} // namespace hardware
