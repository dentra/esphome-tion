#pragma once

#include "esphome/core/component.h"
#include "vport.h"

namespace esphome {
namespace vport {

#define VPORT_LOG(tag, port_name) \
  ESP_LOGCONFIG(tag, "Virtual %s port:", port_name); \
  ESP_LOGCONFIG(tag, "  Update interval: %u s", this->update_interval_ / 1000);

template<typename frame_type> class VPortComponent : public VPort<frame_type>, public PollingComponent {};

}  // namespace vport
}  // namespace esphome
