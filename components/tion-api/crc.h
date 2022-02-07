#pragma once

#include <stdint.h>

namespace dentra {
namespace tion {

uint16_t crc16_ccitt_ffff(const void *data, uint16_t size);

}  // namespace tion
}  // namespace dentra