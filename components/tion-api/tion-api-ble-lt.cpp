#include <utility>
#include <cstring>
#include <cstdlib>

#include "crc.h"
#include "utils.h"
#include "log.h"

#include "tion-api-ble-lt.h"

namespace dentra {
namespace tion {

static const char *const TAG = "tion-api-ble-lt";

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

  BLE_PACKET_MAGIC = 0x3A,
  BLE_MAX_MTU_SIZE = 20,
};

#pragma pack(push, 1)
struct _raw_tion_ble_frame_t {
  uint16_t size;
  uint8_t magic_number;
  uint8_t random_number;                             // always 0xAD
  tion_ble_frame_t<uint8_t[sizeof(uint16_t)]> data;  // sizeof(uint16_t) is crc16 size
};
#pragma pack(pop)

const char *TionBleLtProtocol::get_ble_service() const { return "98f00001-3788-83ea-453e-f52244709ddb"; }
const char *TionBleLtProtocol::get_ble_char_tx() const { return "98f00002-3788-83ea-453e-f52244709ddb"; };
const char *TionBleLtProtocol::get_ble_char_rx() const { return "98f00003-3788-83ea-453e-f52244709ddb"; }

bool TionBleLtProtocol::read_data(const uint8_t *data, size_t size) {
  TION_LOGV(TAG, "Read data: %s", hexencode(data, size).c_str());
  if (data == nullptr || size == 0) {
    TION_LOGW(TAG, "Empy frame data");
    return false;
  }
  const uint8_t packet_type = *data++;
  return this->read_packet_(packet_type, data, --size);
}

bool TionBleLtProtocol::read_packet_(uint8_t packet_type, const uint8_t *data, size_t size) {
  TION_LOGV(TAG, "Read BLE packet 0x%02X: %s", packet_type, hexencode(data, size).c_str());
  if (packet_type == BLE_PACKET_TYPE_FIRST) {
    this->rx_buf_.clear();
    this->rx_buf_.insert(this->rx_buf_.end(), data, data + size);
    return true;
  }

  if (packet_type == BLE_PACKET_TYPE_CURRENT) {
    this->rx_buf_.insert(this->rx_buf_.end(), data, data + size);
    return true;
  }

  if (packet_type == BLE_PACKET_TYPE_FEND) {
    this->read_frame_(data, size);
    return true;
  }

  if (packet_type == BLE_PACKET_TYPE_END) {
    this->rx_buf_.insert(rx_buf_.end(), data, data + size);
    this->read_frame_(this->rx_buf_.data(), this->rx_buf_.size());
    this->rx_buf_.clear();
    this->rx_buf_.shrink_to_fit();
    return true;
  }

  TION_LOGW(TAG, "Unknown BLE packet type %u", packet_type);
  return false;
}

// TODO remove return type
bool TionBleLtProtocol::read_frame_(const void *data, uint32_t size) {
  TION_LOGV(TAG, "Read frame: %s", hexencode(data, size).c_str());
  if (!this->reader) {
    TION_LOGE(TAG, "Reader is not configured");
    return false;
  }

  const _raw_tion_ble_frame_t *frame = static_cast<const _raw_tion_ble_frame_t *>(data);
  if (frame->magic_number != BLE_PACKET_MAGIC) {
    TION_LOGW(TAG, "Invalid magic number: 0x%02X", frame->magic_number);
    return false;
  }
  if (frame->size != size) {
    TION_LOGW(TAG, "Invalid frame size: %u", frame->size);
    return false;
  }
  uint16_t crc = crc16_ccitt_false(frame, size);
  if (crc != 0) {
    TION_LOGW(TAG, "Invalid frame crc: %04X", crc);
    return false;
  }
  this->reader(*reinterpret_cast<const tion_any_ble_frame_t *>(&frame->data),
               frame->size - sizeof(_raw_tion_ble_frame_t) + sizeof(tion_any_ble_frame_t));
  return true;
}

bool TionBleLtProtocol::write_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  TION_LOGV(TAG, "Write frame 0x%04X: %s", frame_type, hexencode(frame_data, frame_data_size).c_str());

  uint8_t tx_buf[frame_data_size + sizeof(_raw_tion_ble_frame_t)];
  auto tx_frame = reinterpret_cast<_raw_tion_ble_frame_t *>(tx_buf);

  tx_frame->magic_number = BLE_PACKET_MAGIC;
  tx_frame->random_number = 0xAD;
  tx_frame->size = sizeof(tx_buf);
  tx_frame->data.type = frame_type;
  tx_frame->data.ble_request_id = 1;  // TODO возможно можно инкрементировать и проверять в ответе
  std::memcpy(tx_frame->data.data, frame_data, frame_data_size);

  uint16_t crc = __builtin_bswap16(crc16_ccitt_false(tx_frame, sizeof(tx_buf) - sizeof(crc)));
  std::memcpy(&tx_frame->data.data[frame_data_size], &crc, sizeof(crc));

  return this->write_packet_(tx_frame, sizeof(tx_buf));
}

bool TionBleLtProtocol::write_packet_(const void *data, uint16_t size) const {
  TION_LOGV(TAG, "Write BLE packet: %s", hexencode(data, size).c_str());

  if (!this->writer) {
    TION_LOGE(TAG, "Writer is not configured");
    return false;
  }

  size_t data_packet_size = BLE_MAX_MTU_SIZE - 1;
  const uint8_t *data_ptr = static_cast<const uint8_t *>(data);

  uint8_t buf[BLE_MAX_MTU_SIZE];

  data_packet_size = size > data_packet_size ? data_packet_size : size;
  size -= data_packet_size;
  buf[0] = size ? BLE_PACKET_TYPE_FIRST : BLE_PACKET_TYPE_FEND;
  std::memcpy(&buf[1], data_ptr, data_packet_size);
  data_ptr += data_packet_size;

  if (!this->writer(buf, data_packet_size + 1)) {
    TION_LOGW(TAG, "Can't write packet");
    return false;
  }

  while (size) {
    data_packet_size = size > data_packet_size ? data_packet_size : size;
    size -= data_packet_size;
    buf[0] = size ? BLE_PACKET_TYPE_CURRENT : BLE_PACKET_TYPE_END;
    std::memcpy(&buf[1], data_ptr, data_packet_size);
    data_ptr += data_packet_size;

    if (!this->writer(buf, data_packet_size + 1)) {
      TION_LOGW(TAG, "Can't write packet");
      return false;
    }
  }

  return true;
}

}  // namespace tion
}  // namespace dentra
