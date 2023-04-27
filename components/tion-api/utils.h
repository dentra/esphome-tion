#pragma once

#include <string>

#define PACKED __attribute__((packed))

#ifdef TION_ESPHOME
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"
#define tion_hexencode(data, size) esphome::format_hex_pretty(data, size)
#define tion_yield() esphome::yield()
#else
#define tion_yield()
#endif

#if !__has_builtin(__builtin_bswap16)
#include <byteswap.h>
#define __builtin_bswap16 __bswap_16
#endif

namespace dentra {
namespace tion {

#ifndef TION_ESPHOME
std::string tion_hexencode(const void *data, uint32_t size);
#endif

inline std::string hexencode(const void *data, uint32_t size) {
  return tion_hexencode(static_cast<const uint8_t *>(data), size);
}

}  // namespace tion
}  // namespace dentra
