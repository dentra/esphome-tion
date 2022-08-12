#include <utility>
#include <cstring>
#include <cstdlib>

#include "crc.h"
#include "utils.h"
#include "log.h"

#include "tion-api-ble-3s.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-ble-3s";

enum : uint8_t { FRAME_MAGIC = 0x5A };

#pragma pack(push, 1)
struct tion3s_frame_t {
  uint16_t type;
  uint8_t data[17];
  uint8_t magic;
};
#pragma pack(pop)

const char *TionBle3sProtocol::get_ble_service() const { return "6e400001-b5a3-f393-e0a9-e50e24dcca9e"; }
const char *TionBle3sProtocol::get_ble_char_tx() const { return "6e400002-b5a3-f393-e0a9-e50e24dcca9e"; };
const char *TionBle3sProtocol::get_ble_char_rx() const { return "6e400003-b5a3-f393-e0a9-e50e24dcca9e"; }

bool TionBle3sProtocol::read_data(const uint8_t *data, size_t size) {
  TION_LOGV(TAG, "Read data: %s", hexencode(data, size).c_str());
  if (data == nullptr || size == 0) {
    TION_LOGW(TAG, "Empy frame data");
    return false;
  }
  if (size != sizeof(tion3s_frame_t)) {
    TION_LOGW(TAG, "Invalid frame size %u", size);
    return false;
  }
  auto frame = reinterpret_cast<const tion3s_frame_t *>(data);
  if (frame->magic != FRAME_MAGIC) {
    TION_LOGW(TAG, "Invalid frame crc %02X", data[size - 1]);
    return false;
  }
  return this->read_frame(frame->type, frame->data, sizeof(frame->data));
}

bool TionBle3sProtocol::write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) const {
  tion3s_frame_t frame{.type = frame_type, .data = {}, .magic = FRAME_MAGIC};
  if (frame_data_size <= sizeof(frame.data)) {
    std::memcpy(frame.data, frame_data, frame_data_size);
  }
  return this->write_data(reinterpret_cast<const uint8_t *>(&frame), sizeof(frame));
}

}  // namespace tion
}  // namespace dentra
