#include <utility>
#include <cstring>
#include <cstdlib>

#include "crc.h"
#include "utils.h"
#include "log.h"

#include "tion-api.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api";

enum tion_packet_type_t : uint8_t {
  BLE_PACKET_TYPE_NONE = 0x01,
  // first packet. 0x00
  BLE_PACKET_TYPE_FIRST = 0 << 6,
  // n-th packet. 0x40
  BLE_PACKET_TYPE_CURRENT = 1 << 6,
  // first and last packet at the same time. 0x80
  BLE_PACKET_TYPE_FEND = 2 << 6,
  // last packet 0xC0.
  BLE_PACKET_TYPE_END = 3 << 6,
};

enum {
  TION_PACKET_MAGIC_NUMBER = 0x3A,
  MAX_MTU_SIZE = 20,
};

#pragma pack(push, 1)
struct tion_frame_t {
  uint16_t size;
  uint8_t magic_number;
  uint8_t random_number;           // always 0xAD
  uint16_t type;                   // frame type
  uint32_t request_id;             // id of request
  uint8_t data[sizeof(uint16_t)];  // crc16 size
};
#pragma pack(pop)

void TionApi::read_data(const uint8_t *data, uint16_t size) {
  TION_LOGV(TAG, "Read data: %s", hexencode(data, size).c_str());
  if (data == nullptr || size == 0) {
    return;
  }
  const uint8_t packet_type = *data++;
  this->read_packet_(packet_type, data, --size);
}

void TionApi::read_packet_(uint8_t packet_type, const uint8_t *data, uint16_t size) {
  TION_LOGV(TAG, "Read packet 0x%02X: %s", packet_type, hexencode(data, size).c_str());
  if (packet_type == BLE_PACKET_TYPE_FIRST) {
    this->rx_buf_.clear();
    this->rx_buf_.insert(this->rx_buf_.end(), data, data + size);
    return;
  }

  if (packet_type == BLE_PACKET_TYPE_CURRENT) {
    this->rx_buf_.insert(this->rx_buf_.end(), data, data + size);
    return;
  }

  if (packet_type == BLE_PACKET_TYPE_FEND) {
    this->read_frame_(data, size);
    return;
  }

  if (packet_type == BLE_PACKET_TYPE_END) {
    this->rx_buf_.insert(rx_buf_.end(), data, data + size);
    this->read_frame_(this->rx_buf_.data(), this->rx_buf_.size());
    this->rx_buf_.clear();
    this->rx_buf_.shrink_to_fit();
    return;
  }

  TION_LOGW(TAG, "Unknown packet type %u", packet_type);
}

void TionApi::read_frame_(const void *data, uint32_t size) {
  TION_LOGV(TAG, "Read frame: %s", hexencode(data, size).c_str());

  const tion_frame_t *frame = static_cast<const tion_frame_t *>(data);
  if (frame->magic_number != TION_PACKET_MAGIC_NUMBER) {
    TION_LOGW(TAG, "Invalid magic number: 0x%02" PRIX8, frame->magic_number);
    return;
  }
  if (frame->size != size) {
    TION_LOGW(TAG, "Invalid frame size: %u", frame->size);
    return;
  }
  uint16_t crc = crc16_ccitt_ffff(frame, size);
  if (crc != 0) {
    TION_LOGW(TAG, "Invalid crc: %x", crc);
    return;
  }

  this->read_(frame->type, frame->data, frame->size - sizeof(tion_frame_t));
}

bool TionApi::write_frame_(uint16_t frame_type, const void *frame_data, uint16_t frame_data_size) const {
  TION_LOGV(TAG, "Write frame 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());

  uint32_t command_id = 0;

  uint16_t frame_size = frame_data_size + sizeof(tion_frame_t);

  uint16_t command_id_size = 0;
  if (command_id != 0) {
    command_id_size = sizeof(command_id);
    frame_size += command_id_size;
  }

  auto tx_frame = static_cast<tion_frame_t *>(std::malloc(frame_size));
  if (tx_frame == nullptr) {
    TION_LOGE(TAG, "Can't allocate %u bytes", frame_size);
    return false;
  }

  tx_frame->type = frame_type;
  tx_frame->size = frame_size;
  tx_frame->request_id = 1;  // !!! можно инкрементировать и проверять в ответе
  tx_frame->magic_number = TION_PACKET_MAGIC_NUMBER;
  tx_frame->random_number = 0xAD;
  std::memcpy(tx_frame->data, frame_data, frame_data_size);

  uint16_t crc = __builtin_bswap16(crc16_ccitt_ffff(tx_frame, frame_size - sizeof(crc)));
  std::memcpy(&tx_frame->data[frame_data_size], &crc, sizeof(crc));

  bool res = this->write_packet_(tx_frame, frame_size);

  std::free(tx_frame);

  return res;
}

bool TionApi::write_packet_(const void *data, uint16_t size) const {
  TION_LOGV(TAG, "Write packet: %s", hexencode(data, size).c_str());

  uint32_t data_packet_size = MAX_MTU_SIZE - 1;
  const uint8_t *data_ptr = static_cast<const uint8_t *>(data);

  uint8_t buf[MAX_MTU_SIZE];

  data_packet_size = size > data_packet_size ? data_packet_size : size;
  size -= data_packet_size;
  buf[0] = size ? BLE_PACKET_TYPE_FIRST : BLE_PACKET_TYPE_FEND;
  std::memcpy(&buf[1], data_ptr, data_packet_size);
  data_ptr += data_packet_size;

  if (!this->write_data(buf, data_packet_size + 1)) {
    TION_LOGW(TAG, "Can't write packet");
    return false;
  }

  while (size) {
    data_packet_size = size > data_packet_size ? data_packet_size : size;
    size -= data_packet_size;
    buf[0] = size ? BLE_PACKET_TYPE_CURRENT : BLE_PACKET_TYPE_END;
    std::memcpy(&buf[1], data_ptr, data_packet_size);
    data_ptr += data_packet_size;

    if (!this->write_data(buf, data_packet_size + 1)) {
      TION_LOGW(TAG, "Can't write packet");
      return false;
    }
  }

  return true;
}

}  // namespace tion
}  // namespace dentra
