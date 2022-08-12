#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

class TionBleWriter {
 public:
};

class TionBleProtocol : public TionProtocol {
 public:
  virtual bool read_data(const uint8_t *data, size_t size) = 0;
  virtual const char *get_ble_service() const = 0;
  virtual const char *get_ble_char_tx() const = 0;
  virtual const char *get_ble_char_rx() const = 0;
};

}  // namespace tion
}  // namespace dentra
