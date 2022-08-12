#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

class TionUartReader {
 public:
  virtual int available() = 0;
  virtual bool peek_byte(uint8_t *data) = 0;
  virtual bool read_array(void *data, size_t size) = 0;
};

class TionUartProtocol : public TionProtocol {
 public:
  void read_uart_data(TionUartReader *io);

  bool write_frame(uint16_t type, const void *data, size_t size) const override;

 protected:
  bool read_frame_(TionUartReader *io);
};

}  // namespace tion
}  // namespace dentra
