#pragma once
#include <cstdint>

namespace dentra {
namespace tion_o2 {

enum : uint16_t {
  // 00
  FRAME_TYPE_CONNECT_REQ = 0x00,
  FRAME_TYPE_CONNECT_RSP = 0x10,

  // 01
  FRAME_TYPE_STATE_GET_REQ = 0x01,
  FRAME_TYPE_STATE_GET_RSP = 0x11,
  // 02
  FRAME_TYPE_STATE_SET_REQ = 0x02,

  // 03
  FRAME_TYPE_DEV_MODE_REQ = 0x03,
  FRAME_TYPE_DEV_MODE_RSP = 0x13,

  // 04
  FRAME_TYPE_SET_WORK_MODE_REQ = 0x04,
  FRAME_TYPE_SET_WORK_MODE_RSP = 0x55,

  // 05
  FRAME_TYPE_TIME_GET_REQ = 0x05,
  FRAME_TYPE_TIME_GET_RSP = 0x15,
  // 06
  FRAME_TYPE_TIME_SET_REQ = 0x06,

  // 07
  FRAME_TYPE_DEV_INFO_REQ = 0x07,
  FRAME_TYPE_DEV_INFO_RSP = 0x17,
};

}  // namespace tion_o2
}  // namespace dentra
