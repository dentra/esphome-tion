#pragma once

#include "esphome/core/preferences.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion/tion.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

class Tion3s : public TionComponent, public TionClimate, public TionBleNode, public TionsApi3s {
 public:
  void setup() override;
  void update() override {
    if (this->pair_state_ > 0) {
      this->parent_->set_enabled(true);
    }
  }

  const esp_bt_uuid_t &get_ble_service() const override;
  const esp_bt_uuid_t &get_ble_char_tx() const override;
  const esp_bt_uuid_t &get_ble_char_rx() const override;

  void read_data(const uint8_t *data, uint16_t size) override { TionsApi3s::read_data(data, size); }
  bool write_data(const uint8_t *data, uint16_t size) const override { return TionBleNode::write_data(data, size); }

  void on_ready() override;
  void read(const tion3s_state_t &state) override;
  bool write_state() override {
    this->publish_state();
    this->dirty_ = true;
    this->parent_->set_enabled(true);
    return true;
  };

  void pair() {
    this->pair_state_ = -1;
    this->parent_->set_enabled(true);
  }

 protected:
  ESPPreferenceObject rtc_;
  bool dirty_{};
  int8_t pair_state_{};  // 0 - not paired, 1 - paired, -1 - pairing
  void update_state_(tion3s_state_t &state);
};

class Tion3sBuzzerSwitch : public switch_::Switch {
 public:
  explicit Tion3sBuzzerSwitch(Tion3s *parent) : parent_(parent) {}
  void write_state(bool state) override {
    this->publish_state(state);
    this->parent_->write_state();
  }

 protected:
  Tion3s *parent_;
};

}  // namespace tion
}  // namespace esphome
