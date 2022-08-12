#include <utility>
#include <cstring>
#include <cstdlib>

#include "crc.h"
#include "utils.h"
#include "log.h"

#include "tion-api-uart.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-uart";

enum {
  FRAME_HEADER = 0x3A,
  FRAME_MAX_SIZE = 0x2A,
};

#pragma pack(push, 1)
struct tion_uart_frame_t {
  uint8_t magic;
  uint16_t size;
  uint16_t type;
  uint8_t data[sizeof(uint16_t)];  // crc16 size
};
#pragma pack(pop)

void TionUartProtocol::read_uart_data(TionUartReader *io) {
  while (io->available() > 0) {
    uint8_t magic;
    if (io->peek_byte(&magic) && magic == FRAME_HEADER) {
      this->read_frame_(io);
    } else {
      io->read_array(&magic, 1);
      TION_LOGW(TAG, "Unxepected byte: 0x%02X", magic);
    }
  }
}

bool TionUartProtocol::read_frame_(TionUartReader *io) {
  uint8_t buf[FRAME_MAX_SIZE];
  if (!io->read_array(buf, 1)) {
    TION_LOGW(TAG, "Failed read frame magic");
    return false;
  }

  auto frame = reinterpret_cast<tion_uart_frame_t *>(buf);
  if (!io->read_array(&frame->size, sizeof(frame->size))) {
    TION_LOGW(TAG, "Failed read frame size");
    return false;
  }

  if (frame->size < sizeof(tion_uart_frame_t) || frame->size > FRAME_MAX_SIZE) {
    TION_LOGW(TAG, "Invalid frame size %u", frame->size);
    return false;
  }

  auto tail_size = frame->size - sizeof(frame->size) - sizeof(frame->magic);
  if (!io->read_array(&frame->type, tail_size)) {
    TION_LOGW(TAG, "Failed read frame data");
    return false;
  }

  TION_LOGV(TAG, "Read data: %s", hexencode(frame, frame->size).c_str());

  auto crc = dentra::tion::crc16_ccitt_false(frame, frame->size);
  if (crc != 0) {
    TION_LOGW(TAG, "Invalid frame CRC %04X ", crc);
    return false;
  }

  auto frame_data_size = frame->size - sizeof(tion_uart_frame_t);
  return this->read_frame(frame->type, frame->data, frame_data_size);
}

bool TionUartProtocol::write_frame(uint16_t type, const void *data, size_t size) const {
  auto frame_size = sizeof(tion_uart_frame_t) + size;
  if (frame_size > FRAME_MAX_SIZE) {
    TION_LOGW(TAG, "Frame size if to large: %u", size);
    return false;
  }

  uint8_t frame_buf[FRAME_MAX_SIZE];
  auto frame = reinterpret_cast<tion_uart_frame_t *>(frame_buf);
  frame->magic = FRAME_HEADER;
  frame->size = frame_size;
  frame->type = type;

  std::memcpy(frame->data, data, size);

  uint16_t crc = __builtin_bswap16(crc16_ccitt_false(frame, frame_size - sizeof(crc)));
  std::memcpy(&frame->data[size], &crc, sizeof(crc));

  return this->write_data(frame_buf, frame_size);
}

}  // namespace tion
}  // namespace dentra
