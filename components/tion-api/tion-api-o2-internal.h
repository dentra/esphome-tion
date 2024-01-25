#pragma once
#include <cstdint>

namespace dentra {
namespace tion_o2 {

enum : uint16_t {
  FRAME_TYPE_CONNECT_REQ_00 = 0x00,
  FRAME_TYPE_CONNECT_RSP_10 = 0x10,
  FRAME_TYPE_DEV_INFO_REQ_07 = 0x07,
  FRAME_TYPE_DEV_INFO_RSP_17 = 0x17,
  FRAME_TYPE_STATE_REQ_01 = 0x01,
  FRAME_TYPE_STATE_RSP_11 = 0x11,
  FRAME_TYPE_HEARTBEAT_REQ_03 = 0x03,
  FRAME_TYPE_HEARTBEAT_RSP_13 = 0x13,
  FRAME_TYPE_SET_WORK_MODE_REQ_04 = 0x04,
  FRAME_TYPE_SET_WORK_MODE_RSP_55 = 0x55,
};

}  // namespace tion_o2
}  // namespace dentra
