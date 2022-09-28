#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

class TionUartReader {
 public:
  virtual int available() = 0;
  virtual bool read_array(void *data, size_t size) = 0;
};

class TionUartProtocol : public TionProtocol {
  enum {
    FRAME_MAX_SIZE = 0x2A,
  };

 public:
  void read_uart_data(TionUartReader *io);

  bool write_frame(uint16_t type, const void *data, size_t size);

 protected:
  /// Reads a frame starting with size for hw uart or continue reading for sw uart
  bool read_frame_(TionUartReader *io);
#ifndef TION_HW_UART_READER
  uint8_t buf_[FRAME_MAX_SIZE]{};
  void reset_buf_();
#endif
};

}  // namespace tion
}  // namespace dentra
