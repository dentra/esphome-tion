#pragma once

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"

namespace esphome {
namespace switch_ {

#define LOG_SWITCH(prefix, type, obj) \
  if ((obj) != nullptr) { \
    ESP_LOGCONFIG(TAG, "%s%s '%s'", prefix, LOG_STR_LITERAL(type), (obj)->get_name().c_str()); \
    if (!(obj)->get_icon().empty()) { \
      ESP_LOGCONFIG(TAG, "%s  Icon: '%s'", prefix, (obj)->get_icon().c_str()); \
    } \
    if ((obj)->assumed_state()) { \
      ESP_LOGCONFIG(TAG, "%s  Assumed State: YES", prefix); \
    } \
    if ((obj)->is_inverted()) { \
      ESP_LOGCONFIG(TAG, "%s  Inverted: YES", prefix); \
    } \
    if (!(obj)->get_device_class().empty()) { \
      ESP_LOGCONFIG(TAG, "%s  Device Class: '%s'", prefix, (obj)->get_device_class().c_str()); \
    } \
  }

class Switch : public EntityBase {
 public:
  virtual ~Switch() {}
  bool state;
  void publish_state(bool state) { this->state = state; }
  bool is_inverted() const { return false; }
  bool assumed_state() const { return false; }
  std::string get_icon() const { return {}; }
  std::string get_device_class() const { return {}; }

 protected:
  virtual void write_state(bool state) = 0;
};

}  // namespace switch_
}  // namespace esphome
