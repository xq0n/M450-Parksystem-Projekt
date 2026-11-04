#pragma once

#include <Arduino.h>
#include <ctype.h>
#include <string.h>

// Vereinheitlicht Kennzeichen wie "lu123456" und "LU 123456".
// Erlaubt 2-3 Buchstaben und 1-6 Ziffern, wie beim Testkennzeichen LU 123456.
inline bool normalizePlate(const String &raw, char *output, size_t outputSize) {
  String compact;
  compact.reserve(raw.length());

  for (size_t i = 0; i < raw.length(); ++i) {
    const char c = raw[i];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      continue;
    }
    if (!isalpha(static_cast<unsigned char>(c)) &&
        !isdigit(static_cast<unsigned char>(c))) {
      return false;
    }
    compact += static_cast<char>(toupper(static_cast<unsigned char>(c)));
  }

  size_t letterCount = 0;
  while (letterCount < compact.length() &&
         isalpha(static_cast<unsigned char>(compact[letterCount]))) {
    ++letterCount;
  }
  const size_t digitCount = compact.length() - letterCount;
  if (letterCount < 2 || letterCount > 3 || digitCount < 1 || digitCount > 6) {
    return false;
  }
  for (size_t i = letterCount; i < compact.length(); ++i) {
    if (!isdigit(static_cast<unsigned char>(compact[i]))) {
      return false;
    }
  }

  const size_t requiredSize = letterCount + 1 + digitCount + 1;
  if (outputSize < requiredSize) {
    return false;
  }
  memcpy(output, compact.c_str(), letterCount);
  output[letterCount] = ' ';
  memcpy(output + letterCount + 1, compact.c_str() + letterCount, digitCount);
  output[requiredSize - 1] = '\0';
  return true;
}

inline String formatUptime(uint32_t uptimeMs) {
  const uint32_t totalSeconds = uptimeMs / 1000UL;
  const uint32_t hours = totalSeconds / 3600UL;
  const uint8_t minutes = (totalSeconds / 60UL) % 60UL;
  const uint8_t seconds = totalSeconds % 60UL;
  char text[20];
  snprintf(text, sizeof(text), "%lu:%02u:%02u",
           static_cast<unsigned long>(hours), minutes, seconds);
  return String(text);
}

inline String formatDuration(uint32_t durationMs) {
  const uint32_t totalMinutes = durationMs / 60000UL;
  const uint32_t hours = totalMinutes / 60UL;
  const uint8_t minutes = totalMinutes % 60UL;
  char text[32];
  snprintf(text, sizeof(text), "%lu h %02u min",
           static_cast<unsigned long>(hours), minutes);
  return String(text);
}

