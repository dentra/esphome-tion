#pragma once

#include "tion-api-ble.h"

namespace dentra {
namespace tion {

class TionBleLtProtocol : public TionBleProtocol {
 public:
  bool read_data(const uint8_t *data, size_t size) override;

  bool write_frame(uint16_t type, const void *data, size_t size) const override;

  const char *get_ble_service() const;
  const char *get_ble_char_tx() const;
  const char *get_ble_char_rx() const;

 protected:
  std::vector<uint8_t> rx_buf_;

  bool read_packet_(uint8_t packet_type, const uint8_t *data, size_t size);
  bool write_packet_(const void *data, uint16_t size) const;
  bool read_frame_(const void *data, uint32_t size);
};

}  // namespace tion
}  // namespace dentra
