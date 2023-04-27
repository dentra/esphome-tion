#include <cstring>
#include "log.h"
#include "utils.h"
#include "tion-api-uart-3s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-uart-3s";

#pragma pack(push, 1)
struct tion3s_frame_t {
  enum : uint8_t {
    FRAME_MAGIC_REQ = 0x3D,
    FRAME_MAGIC_RSP = 0xB3,
    FRAME_MAGIC = 0x5A,
    FRAME_DATA_SIZE = 17,
  };
  union {
    struct {
      uint8_t head;
      uint8_t type;
      uint8_t data[FRAME_DATA_SIZE];
    } rx;
    tion_frame_t<uint8_t[FRAME_DATA_SIZE]> data;
  };
  uint8_t magic;
};
#pragma pack(pop)

void TionUartProtocol3s::read_uart_data(TionUartReader *io) {
  if (!this->reader) {
    TION_LOGE(TAG, "Reader is not configured");
    return;
  }

  while (io->available() > 0) {
    if (!this->read_frame_(io)) {
      break;
    }
    tion_yield();
  }
}

bool TionUartProtocol3s::read_frame_(TionUartReader *io) {
  auto frame = reinterpret_cast<tion3s_frame_t *>(this->buf_);

  if (frame->rx.head != tion3s_frame_t::FRAME_MAGIC_RSP) {
    if (io->available() < sizeof(frame->rx.head)) {
      // do not flood log while waiting magic
      // TION_LOGV(TAG, "Waiting frame magic");
      return false;
    }
    if (!io->read_array(&frame->rx.head, sizeof(frame->rx.head))) {
      TION_LOGW(TAG, "Failed read frame head");
      return true;
    }
    if (frame->rx.head != tion3s_frame_t::FRAME_MAGIC_RSP) {
      TION_LOGW(TAG, "Unxepected byte: 0x%02X", frame->rx.head);
      return true;
    }
  }

  if (frame->rx.type == 0) {
    if (io->available() < sizeof(frame->rx.type)) {
      TION_LOGV(TAG, "Waiting frame type");
      return false;
    }
    if (!io->read_array(&frame->rx.type, sizeof(frame->rx.type))) {
      TION_LOGW(TAG, "Failed read frame type");
      this->reset_buf_();
      return true;
    }
  }

  constexpr uint32_t tail_size = sizeof(frame->rx.data) + sizeof(frame->magic);
  if (io->available() < tail_size) {
    TION_LOGV(TAG, "Waiting frame data %u of %u", io->available(), tail_size);
    return false;
  }

  if (!io->read_array(&frame->rx.data, tail_size)) {
    TION_LOGW(TAG, "Failed read frame data");
    this->reset_buf_();
    return true;
  }

  TION_LOGV(TAG, "Read data: %s", hexencode(&frame->data, sizeof(frame->data)).c_str());

  if (frame->magic != tion3s_frame_t::FRAME_MAGIC) {
    TION_LOGW(TAG, "Invlid frame magic %02X", frame->magic);
    this->reset_buf_();
    return true;
  }

  tion_yield();
  this->reader(*reinterpret_cast<const tion_any_frame_t *>(&frame->data), sizeof(frame->data));
  this->reset_buf_();
  return false;
}

void TionUartProtocol3s::reset_buf_() { std::memset(this->buf_, 0, sizeof(this->buf_)); }

bool TionUartProtocol3s::write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  if (!this->writer) {
    TION_LOGE(TAG, "Writer is not configured");
    return false;
  }

  tion3s_frame_t frame{.data = {.type = frame_type, .data = {}}, .magic = tion3s_frame_t::FRAME_MAGIC};
  if (frame_data_size <= sizeof(frame.data.data)) {
    std::memcpy(frame.data.data, frame_data, frame_data_size);
  }

  return this->writer(reinterpret_cast<uint8_t *>(&frame), sizeof(frame));
}

}  // namespace tion
}  // namespace dentra
