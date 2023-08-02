#pragma once

#include "tion-api-3s.h"
#include "tion-api-uart.h"

namespace dentra {
namespace tion {

// 20 = sizeof(tion3s_frame_t)
class TionUartProtocol3s : public TionUartProtocolBase<20> {
 public:
  void read_uart_data(TionUartReader *io);

  bool write_frame(uint16_t type, const void *data, size_t size);

 protected:
  /// Reads a frame starting with size for hw uart or continue reading for sw uart
  read_frame_result_t read_frame_(TionUartReader *io);
  uint8_t head_type_{FRAME_MAGIC_RSP};
};

}  // namespace tion
}  // namespace dentra
