#include "esphome/core/log.h"
#include "tion_ble_3s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_ble_3s";

void Tion3sBLEVPortBase::dump_config() {
  this->dump_settings(TAG);
  ESP_LOGCONFIG(TAG, "  Always Pair (experimental): %s", ONOFF(this->experimental_always_pair_));
  ESP_LOGCONFIG(TAG, "  Paired: %s", ONOFF(this->pair_state_ > 0));
}

void Tion3sBLEVPortBase::setup() {
  this->rtc_ = global_preferences->make_preference<int8_t>(fnv1_hash("tion_3s"), true);
  int8_t loaded{};
  if (this->rtc_.load(&loaded)) {
    this->pair_state_ = loaded;
  }
}

void Tion3sBLEVPortBase::update() {
  if (!this->is_connected() || this->pair_state_ > 0) {
    TionBLEVPortBase::update();
  } else {
    ESP_LOGW(TAG, "Pairing required. [pair_state: %d, is_connected: %s", this->pair_state_,
             YESNO(this->is_connected()));
    this->disconnect();
  }
}

void Tion3sBLEVPortBase::pair() {
  this->pair_state_ = -1;
  this->connect();
}

void Tion3sBLEVPortBase::reset_pair() {
  this->pair_state_ = 0;
  this->rtc_.save(&this->pair_state_);
  this->disconnect();
}

void Tion3sBLEVPortBase::on_ble_ready() {
  if (this->api_ == nullptr) {
    ESP_LOGD(TAG, "Tion API is not configured");
    return;
  }

  if (this->pair_state_ == 0) {
    ESP_LOGD(TAG, "Not paired yet");
    return;
  }

  if (this->pair_state_ < 0) {
    bool res = this->api_->pair();
    if (res) {
      this->pair_state_ = 1;
      this->rtc_.save(&this->pair_state_);
    }
    ESP_LOGD(TAG, "Pairing complete: %s", YESNO(res));
    return;
  }

  if (this->experimental_always_pair_) {
    this->api_->pair();
  }

  TionBLEVPortBase::on_ble_ready();
}

}  // namespace tion
}  // namespace esphome
