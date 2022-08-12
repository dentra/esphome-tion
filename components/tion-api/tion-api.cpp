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

bool TionApiBase::write_frame(uint16_t type, const void *data, size_t size) const {
  TION_LOGV(TAG, "Write frame 0x%04X: %s", type, hexencode(data, size).c_str());
  if (this->writer_ == nullptr) {
    TION_LOGE(TAG, "Writer is not configured");
    return false;
  }
  return this->writer_->write_frame(type, data, size);
}

// void TionProtocol::read_frame(uint16_t type, const void *data, size_t size) const {
//   TION_LOGV(TAG, "Read frame 0x%04X: %s", type, hexencode(data, size).c_str());
//   if (this->reader_ == nullptr) {
//     TION_LOGE(TAG, "Reader is not configured");
//     return;
//   }
//   this->reader_->read_frame(type, data, size);
// }

}  // namespace tion
}  // namespace dentra
