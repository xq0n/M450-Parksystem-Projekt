#pragma once
#include "parking_common.h"
#include <stdio.h>

namespace parking {
// Fixed bounds and strict framing prevent partial/ambiguous requests reaching handlers.
class HttpRequest {
 public:
  enum State { Reading, Complete, Bad };
  State state = Reading;
  char method[5] = {}, path[64] = {};
  State feed(char c) {
    if (state != Reading) return state;
    if (!c || used_ == sizeof(data_) - 1) return state = Bad;
    data_[used_++] = c; data_[used_] = 0;
    if (!bodyAt_ && used_ >= 4 && !memcmp(data_ + used_ - 4, "\r\n\r\n", 4)) {
      bodyAt_ = used_;
      if (!parseHeaders()) return state = Bad;
    }
    if (bodyAt_ && used_ == bodyAt_ + length_) state = Complete;
    return state;
  }
  bool arg(const char* name, char* out, size_t capacity) const {
    if (state != Complete || !capacity) return false;
    bool found = false; out[0] = 0;
    const char* start = data_ + bodyAt_;
    while (*start) {
      const char* end = strchr(start, '&'); if (!end) end = start + strlen(start);
      const char* eq = static_cast<const char*>(memchr(start, '=', end - start));
      if (!eq) return false;
      char key[40]; if (!unescape(start, eq, key, sizeof(key))) return false;
      if (!strcmp(key, name)) {
        if (found || !unescape(eq + 1, end, out, capacity)) return false;
        found = true;
      }
      start = *end ? end + 1 : end;
    }
    return found;
  }
 private:
  static int hex(char c) { if (c >= '0' && c <= '9') return c - '0'; if (c >= 'A' && c <= 'F') return c - 'A' + 10; if (c >= 'a' && c <= 'f') return c - 'a' + 10; return -1; }
  static bool unescape(const char* p, const char* end, char* out, size_t cap) {
    size_t n = 0;
    while (p < end) {
      char c = *p++;
      if (c == '+') c = ' ';
      else if (c == '%') { if (end - p < 2 || hex(p[0]) < 0 || hex(p[1]) < 0) return false; c = char(hex(p[0]) * 16 + hex(p[1])); p += 2; }
      if (static_cast<unsigned char>(c) < 32 || c == 127 || n + 1 >= cap) return false;
      out[n++] = c;
    }
    out[n] = 0; return true;
  }
  bool parseHeaders() {
    char* line = strstr(data_, "\r\n"); if (!line) return false;
    *line = 0;
    char version[12] = {}, extra;
    if (sscanf(data_, "%4s %63s %11s %c", method, path, version, &extra) != 3 ||
        (strcmp(method, "GET") && strcmp(method, "POST")) || strcmp(version, "HTTP/1.1") || path[0] != '/') return false;
    bool lengthSeen = false, form = false;
    char* p = line + 2;
    while (*p != '\r') {
      line = strstr(p, "\r\n"); if (!line) return false; *line = 0;
      char* colon = strchr(p, ':'); if (!colon) return false; *colon = 0;
      for (char* q = p; *q; ++q) if (*q >= 'A' && *q <= 'Z') *q += 'a' - 'A';
      char* value = colon + 1; while (*value == ' ' || *value == '\t') ++value;
      if (!strcmp(p, "content-length")) {
        uint32_t len = 0;
        if (lengthSeen || !unsignedNumber(value, 0, 512, len)) return false;
        lengthSeen = true; length_ = len;
      } else if (!strcmp(p, "transfer-encoding")) return false;
      else if (!strcmp(p, "content-type")) form = !strcmp(value, "application/x-www-form-urlencoded");
      p = line + 2;
    }
    if (!strcmp(method, "POST") && (!lengthSeen || !form)) return false;
    if (!strcmp(method, "GET") && length_) return false;
    return bodyAt_ + length_ < sizeof(data_);
  }
  char data_[2049] = {};
  size_t used_ = 0, bodyAt_ = 0, length_ = 0;
};
} // namespace parking
