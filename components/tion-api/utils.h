#pragma once

#include <cstdint>
#include <string>

#ifdef TION_ESPHOME
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#include "esphome/core/optional.h"
#else
#include <optional>
#endif

#if !__has_builtin(__builtin_bswap16)
#include <byteswap.h>
#endif

namespace dentra {
namespace tion {

#ifdef TION_ESPHOME
inline std::string tion_hexencode(const void *data, uint32_t size) {
  return esphome::format_hex_pretty(static_cast<const uint8_t *>(data), size);
}
inline void yield() { esphome::yield(); }
inline uint32_t millis() { return esphome::millis(); }
inline void delay(uint32_t ms) { esphome::delay(ms); }
using esphome::optional;
using esphome::str_sprintf;
// на ESP8266 buf_append_printf - макрос, оборачивающий формат в PSTR и вызывающий buf_append_printf_p
#ifdef USE_ESP8266
using esphome::buf_append_printf_p;
#else
using esphome::buf_append_printf;
#endif
#else
#define PACKED __attribute__((packed))
std::string tion_hexencode(const void *data, uint32_t size);
inline void yield() {}
// weak, tests may override it with a fake clock
uint32_t millis();
// weak, no-op by default
void delay(uint32_t ms);
#ifndef ONOFF
#define ONOFF(b) ((b) ? "ON" : "OFF")
#endif
using std::optional;
std::string __attribute__((format(printf, 1, 2))) str_sprintf(const char *fmt, ...);
// добавляет форматированную строку в buf с позиции pos, возвращает новую позицию (не больше size)
size_t __attribute__((format(printf, 4, 5)))
buf_append_printf(char *buf, size_t size, size_t pos, const char *fmt, ...);
#endif

#if !__has_builtin(__builtin_bswap16)
#define __builtin_bswap16 __bswap_16
#endif

#define hex_cstr(data, size) tion_hexencode(data, size).c_str()

const char *get_flag_bits(uint8_t flags);

}  // namespace tion
}  // namespace dentra
