#pragma once
#include "parking/input.h"
#include "parking/http_parser.h"

namespace parking {
// Pure adapter: the same form decoding is usable on the Pico and in hosttests.
class HttpPlateInput final : public PlateInput {
 public:
  explicit HttpPlateInput(const HttpRequest& request) : request_(request) {}
  bool read(char* plate, size_t size) const override {
    char raw[33];
    return request_.arg("plate", raw, sizeof(raw)) && normalizePlate(raw, plate, size);
  }

 private:
  const HttpRequest& request_;
};
} // namespace parking
