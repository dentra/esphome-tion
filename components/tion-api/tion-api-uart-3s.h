#pragma once

#include "tion-api-3s.h"
#include "tion-api-uart.h"

namespace dentra {
namespace tion {

class TionUartProtocol3s : public TionProtocol<tion_any_frame_t> {
  enum {
    FRAME_MAX_SIZE = 20  // sizeof(tion3s_frame_t)
  };

 public:
  void read_uart_data(TionUartReader *io);

  bool write_frame(uint16_t type, const void *data, size_t size);

 protected:
  /// Reads a frame starting with size for hw uart or continue reading for sw uart
  bool read_frame_(TionUartReader *io);
  uint8_t buf_[FRAME_MAX_SIZE]{};
  void reset_buf_();
};

}  // namespace tion
}  // namespace dentra
