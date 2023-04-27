#include <utility>
#include <cstring>
#include <cstdlib>

#include "crc.h"
#include "utils.h"
#include "log.h"

#include "tion-api-uart-lt.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-uart-lt";

enum {
  FRAME_HEADER = 0x3A,
};

#pragma pack(push, 1)
struct tion_uart_frame_t {
  uint8_t magic;
  uint16_t size;
  tion_frame_t<uint8_t[sizeof(uint16_t)]> data;  // sizeof(uint16_t) is crc16 size
};
#pragma pack(pop)

void TionUartProtocolLt::read_uart_data(TionUartReader *io) {
  if (!this->reader) {
    TION_LOGE(TAG, "Reader is not configured");
    return;
  }
#ifdef TION_HW_UART_READER
  while (io->available() > 0) {
    uint8_t magic;
    if (io->read_array(&magic, 1) && magic == FRAME_HEADER) {
      if (!this->read_frame_(io)) {
        tion_yield();
        break;
      }
    } else {
      TION_LOGW(TAG, "Unxepected byte: 0x%02X", magic);
    }
    tion_yield();
  }
#else
  while (io->available() > 0) {
    if (!this->read_frame_(io)) {
      break;
    }
    tion_yield();
  }
#endif
}

#ifdef TION_HW_UART_READER
bool TionUartProtocolLt::read_frame_(TionUartReader *io) {
  uint8_t buf[FRAME_MAX_SIZE]{FRAME_HEADER};

  if (!io->read_array(&frame->size, sizeof(frame->size))) {
    TION_LOGW(TAG, "Failed read frame size");
    return true;
  }

  if (frame->size < sizeof(tion_uart_frame_t) || frame->size > FRAME_MAX_SIZE) {
    TION_LOGW(TAG, "Invalid frame size %u", frame->size);
    return true;
  }

  auto tail_size = frame->size - sizeof(frame->size) - sizeof(frame->magic);
  if (!io->read_array(&frame->type, tail_size)) {
    TION_LOGW(TAG, "Failed read frame data");
    this->reset_buf_();
    return true;
  }

  TION_LOGV(TAG, "Read data: %s", hexencode(frame, frame->size).c_str());

  auto crc = dentra::tion::crc16_ccitt_false(frame, frame->size);
  if (crc != 0) {
    TION_LOGW(TAG, "Invalid CRC %04X for frame %s", crc, hexencode(frame, frame->size).c_str());
    return true;
  }

  tion_yield();

  auto frame_data_size = frame->size - sizeof(tion_uart_frame_t);
  this->reader(frame->type, frame->data, frame_data_size);
  return true;
}
#else
bool TionUartProtocolLt::read_frame_(TionUartReader *io) {
  auto frame = reinterpret_cast<tion_uart_frame_t *>(this->buf_);
  if (frame->magic != FRAME_HEADER) {
    if (io->available() < sizeof(frame->magic)) {
      // do not flood log while waiting magic
      // TION_LOGV(TAG, "Waiting frame magic");
      return false;
    }
    if (!io->read_array(&frame->magic, sizeof(frame->magic))) {
      TION_LOGW(TAG, "Failed read frame magic");
      return true;
    }
    if (frame->magic != FRAME_HEADER) {
      TION_LOGW(TAG, "Unxepected byte: 0x%02X", frame->magic);
      return true;
    }
  }

  if (frame->size == 0) {
    if (io->available() < sizeof(frame->size)) {
      TION_LOGV(TAG, "Waiting frame size %u of %u", io->available(), sizeof(frame->size));
      return false;
    }
    if (!io->read_array(&frame->size, sizeof(frame->size))) {
      TION_LOGW(TAG, "Failed read frame size");
      this->reset_buf_();
      return true;
    }
  }

  if (frame->size < sizeof(tion_uart_frame_t) || frame->size > FRAME_MAX_SIZE) {
    TION_LOGW(TAG, "Invalid frame size %u", frame->size);
    this->reset_buf_();
    return true;
  }

  auto tail_size = frame->size - sizeof(frame->size) - sizeof(frame->magic);
  if (io->available() < tail_size) {
    TION_LOGV(TAG, "Waiting frame data %u of %u", io->available(), tail_size);
    return false;
  }
  if (!io->read_array(&frame->data.type, tail_size)) {
    TION_LOGW(TAG, "Failed read frame data");
    this->reset_buf_();
    return true;
  }

  TION_LOGV(TAG, "Read data: %s", hexencode(frame, frame->size).c_str());

  auto crc = dentra::tion::crc16_ccitt_false(frame, frame->size);
  if (crc != 0) {
    TION_LOGW(TAG, "Invalid CRC %04X for frame %s", crc, hexencode(frame, frame->size).c_str());
    this->reset_buf_();
    // let perfrom read next frame on next loop
    return false;
  }

  tion_yield();

  auto frame_data_size = frame->size - sizeof(tion_uart_frame_t) + sizeof(tion_any_frame_t);
  this->reader(*reinterpret_cast<const tion_any_frame_t *>(&frame->data), frame_data_size);
  this->reset_buf_();
  // let perfrom read next frame on next loop
  return false;
}

void TionUartProtocolLt::reset_buf_() { std::memset(this->buf_, 0, sizeof(this->buf_)); }
#endif

bool TionUartProtocolLt::write_frame(uint16_t type, const void *data, size_t size) {
  if (!this->writer) {
    TION_LOGE(TAG, "Writer is not configured");
    return false;
  }

  auto frame_size = sizeof(tion_uart_frame_t) + size;
  if (frame_size > FRAME_MAX_SIZE) {
    TION_LOGW(TAG, "Frame size is to large: %u", size);
    return false;
  }

  uint8_t frame_buf[FRAME_MAX_SIZE];
  auto frame = reinterpret_cast<tion_uart_frame_t *>(frame_buf);
  frame->magic = FRAME_HEADER;
  frame->size = frame_size;
  frame->data.type = type;

  std::memcpy(frame->data.data, data, size);
  uint16_t crc = __builtin_bswap16(crc16_ccitt_false(frame, frame_size - sizeof(crc)));
  std::memcpy(&frame->data.data[size], &crc, sizeof(crc));

  return this->writer(frame_buf, frame_size);
}

}  // namespace tion
}  // namespace dentra
