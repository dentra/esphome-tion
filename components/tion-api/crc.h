#pragma once

#include <cstdint>

namespace dentra {
namespace tion {

uint16_t crc16_ccitt_false(uint16_t init, const void *data, uint16_t size);

/// CRC-16/CCITT-FALSE with 0xFFFF initial value. The result is in big-endian byte order.
inline uint16_t crc16_ccitt_false(const void *data, uint16_t size) { return crc16_ccitt_false(0xFFFF, data, size); }

}  // namespace tion
}  // namespace dentra
