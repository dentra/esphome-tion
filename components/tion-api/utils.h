#pragma once

#include <string>

#define PACKED __attribute__((packed))

#ifdef TION_ESPHOME
#include "esphome/core/helpers.h"
#define tion_hexencode esphome::format_hex_pretty
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
