#if 0
#include <cinttypes>
#include "esphome/core/log.h"

#include "tion_boost.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_boost";
static const char *const ASH_BOOST = TAG;

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

void TionBoost::dump_config() {
  ESP_LOGCONFIG(TAG, "Tion Boost %s:", this->get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Update interval: %.1f s", this->get_update_interval() / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Boost time: %u min", this->boost_time_);
}

void TionBoost::setup() {
  if (!this->update_interval_) {
    this->mark_failed();
  }

  this->parent_->add_on_state_callback([this](const TionState *state) {
    if (state) {
      this->on_state_(*state);
    }
  });
}

void TionBoost::write_state(bool state) {
  if (this->is_failed()) {
    return;
  }
  if (state) {
    this->boost_enable();
  } else {
    this->boost_cancel();
  }
}

void TionBoost::on_state_(const TionState &state) {
  if (!state.boost_state) {
    return;
  }
  if (!state.power_state || state.fan_speed != this->parent_->traits().max_fan_speed ||
      state.gate_position != TionGatePosition::OUTDOOR) {
    this->boost_cancel();
    return;
  }
}

void TionBoost::boost_enable() {
  if (!this->parent_->state().is_initialized(this->parent_->traits())) {
    ESP_LOGW(TAG, "State is not initialized. Boost was't started.");
    return;
  }

  if (this->parent_->state().boost_state) {
    ESP_LOGW(TAG, "Boost already in progress, time left %" PRIu32 " s", this->parent_->state().boost_time_left);
    return;
  }

  const int boost_time = this->boost_time_ * 60;
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return;
  }

  this->boost_save_.power_state = this->parent_->state().power_state;
  this->boost_save_.heater_state = this->parent_->state().heater_state;
  this->boost_save_.fan_speed = this->parent_->state().fan_speed;
  this->boost_save_.target_temperature = this->parent_->state().target_temperature;
  this->boost_save_.gate_position = this->parent_->state().gate_position;

  ESP_LOGD(TAG, "Schedule boost for %" PRIu32 " s", boost_time);
  this->parent_->api()->update_boost(true, boost_time);
  auto *call = this->parent_->make_call();
  call->set_fan_speed(this->parent_->traits().max_fan_speed);
  call->set_power_state(true);
  call->set_gate_position(TionGatePosition::OUTDOOR);
  call->set_heater_state(this->boost_preset_.heater_state);
  call->set_target_temperature(this->boost_preset_.target_temperature);
  call->perform();

  this->set_interval(ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this]() {
    const int time_left = int(this->parent_->state().boost_time_left) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    ESP_LOGV(TAG, "Boost time left %d s", time_left);
    if (time_left > 0) {
      this->parent_->api()->update_boost(true, time_left);
    } else {
      this->boost_cancel();
    }
  });
}

void TionBoost::boost_cancel() {
  this->cancel_interval(ASH_BOOST);
  if (!this->parent_->state().boost_state) {
  }
  ESP_LOGV(TAG, "Boost finished");
  this->parent_->api()->update_boost(false);
  auto *call = this->parent_->make_call();
  call->set_power_state(this->boost_save_.power_state);
  call->set_heater_state(this->boost_save_.heater_state);
  call->set_fan_speed(this->boost_save_.fan_speed);
  call->set_target_temperature(this->boost_save_.target_temperature);
  call->set_gate_position(this->boost_save_.gate_position);
  call->perform();
}

}  // namespace tion
}  // namespace esphome
#endif
