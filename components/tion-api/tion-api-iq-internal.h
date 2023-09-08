#pragma once

#include <cstdint>

namespace dentra {
namespace tion_iq {

enum {
  FRAME_TYPE_STATE_SET = 0x3230,
  FRAME_TYPE_STATE_RSP = 0x3231,
  FRAME_TYPE_STATE_REQ = 0x3232,

  FRAME_TYPE_DEV_INFO_REQ = 0x3332,
  FRAME_TYPE_DEV_INFO_RSP = 0x3331,
};

}  // namespace tion_iq
}  // namespace dentra
