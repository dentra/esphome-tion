#include <cinttypes>

#include "esphome/core/log.h"
#include "tion_api_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_api_component";
static const char *const STATE_TIMEOUT = "state_timeout";

void TionApiComponent::call_setup() {
  PollingComponent::call_setup();
  if (this->state_timeout_ >= this->get_update_interval()) {
    ESP_LOGW(TAG, "Invalid state timout: %.1f s", this->state_timeout_ / 1000.0f);
    this->state_timeout_ = 0;
  }
  if (this->state_timeout_ == 0) {
    this->has_state_ = true;
  }
}

void TionApiComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "%s:", this->get_component_source());
  ESP_LOGCONFIG(TAG, "  Update interval: %.1f s", this->get_update_interval() / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Force update: %s", ONOFF(this->force_update_));
  ESP_LOGCONFIG(TAG, "  State timeout: %.1f s", this->state_timeout_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Batch timeout: %.1f s", this->batch_timeout_ / 1000.0f);
}

void TionApiComponent::update() {
  this->api_->request_state();
  this->state_check_schedule_();
}

void TionApiComponent::on_state_(const TionState &state, const uint32_t request_id) {
  this->state_check_cancel_();
  this->defer([this]() {
    const auto &state = this->state();
    this->state_callback_.call(&state);
  });
  // this->state_callback_.call(&state);
}

void TionApiComponent::state_check_schedule_() {
  if (this->state_timeout_ > 0) {
    this->set_timeout(STATE_TIMEOUT, this->state_timeout_, [this]() {
      this->has_state_ = false;
      this->state_check_report_(this->state_timeout_);
    });
  } else if (!this->has_state_) {
    this->state_check_report_(this->get_update_interval());
  } else {
    this->has_state_ = false;
  }
}

void TionApiComponent::state_check_report_(uint32_t timeout) {
  ESP_LOGW(TAG, "State was not received in %.1fs", timeout / 1000.0f);
  this->state_callback_.call(nullptr);
}

void TionApiComponent::state_check_cancel_() {
  this->has_state_ = true;
  if (this->state_timeout_ > 0) {
    this->cancel_timeout(STATE_TIMEOUT);
  }
}

dentra::tion::TionStateCall *TionApiComponent::make_call() {
  if (this->batch_call_) {
    ESP_LOGD(TAG, "Continue batch update for %u ms", this->batch_timeout_);
  } else {
    ESP_LOGD(TAG, "Starting batch update for %u ms", this->batch_timeout_);
    this->batch_call_ = new BatchStateCall(this->api_, this, this->batch_timeout_, [this]() { this->batch_write_(); });
  }
  return this->batch_call_;
}

void TionApiComponent::batch_write_() {
  BatchStateCall *batch_call = this->batch_call_;
  this->batch_call_ = nullptr;
  delete batch_call;
  this->state_check_schedule_();
}

}  // namespace tion
}  // namespace esphome
