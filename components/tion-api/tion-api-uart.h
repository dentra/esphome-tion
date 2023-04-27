#pragma once

#include "tion-api.h"

namespace dentra {
namespace tion {

class TionUartReader {
 public:
  virtual int available() = 0;
  virtual bool read_array(void *data, size_t size) = 0;
};

}  // namespace tion
}  // namespace dentra
