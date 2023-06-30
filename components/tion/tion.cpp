#include <cstddef>

#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion";

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

// application scheduler name
static const char *const ASH_BOOST = "tion-boost";

void TionClimateComponentBase::setup() {
  TionComponent::setup();

#ifdef TION_ENABLE_PRESETS
#ifdef USE_API
  constexpr auto presets_count = (sizeof(this->presets_) / sizeof(this->presets_[0]));
  this->rtc_ = global_preferences->make_preference<tion_preset_t[presets_count]>(this->get_object_id_hash());
  if (this->rtc_.load(this->presets_)) {
    ESP_LOGD(TAG, "Presets loaded");
  }
  this->register_service(&TionClimateComponentBase::update_preset, "update_preset",
                         {"preset", "mode", "fan_speed", "target_temperature"});
#endif  // USE_API
#endif  // TION_ENABLE_PRESETS

  auto state = this->restore_state_();
  if (state.has_value()) {
    this->control(state->to_call(this));
  }
}

#ifdef TION_ENABLE_PRESETS
#ifdef USE_API
void TionClimateComponentBase::update_preset_service_(climate::ClimatePreset preset, climate::ClimateMode mode,
                                                      uint8_t fan_speed, int8_t target_temperature) {
  if (this->update_preset(preset, mode, fan_speed, target_temperature)) {
    if (this->rtc_.save(this->presets_)) {
      ESP_LOGD(TAG, "Preset %s was updated", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
    }
  } else {
    ESP_LOGW(TAG, "Preset %s was't updated", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  }
}
#endif  // USE_API

bool TionClimateComponentBase::enable_boost_() {
  auto boost_time = this->get_boost_time();
  if (boost_time == 0) {
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %u s", boost_time);
    this->set_timeout(ASH_BOOST, boost_time * 1000, [this]() { this->cancel_preset_(*this->preset); });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %u s", boost_time);
  this->boost_time_left_->publish_state(static_cast<float>(boost_time));
  this->set_interval(ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this]() {
    int32_t time_left = static_cast<int32_t>(this->boost_time_left_->state) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    ESP_LOGV(TAG, "Boost time left %d s", time_left);
    if (time_left > 0) {
      this->boost_time_left_->publish_state(static_cast<float>(time_left));
    } else {
      this->cancel_preset_(*this->preset);
    }
  });

  return true;
}

void TionClimateComponentBase::cancel_boost_() {
  if (this->boost_time_left_) {
    ESP_LOGV(TAG, "Cancel boost interval");
    this->cancel_interval(ASH_BOOST);
  } else {
    ESP_LOGV(TAG, "Cancel boost timeout");
    this->cancel_timeout(ASH_BOOST);
  }
  this->boost_time_left_->publish_state(NAN);
}
#endif  // TION_ENABLE_PRESETS

void TionClimateComponentBase::dump_settings(const char *TAG, const char *component) const {
  LOG_CLIMATE(component, "", this);
  LOG_UPDATE_INTERVAL(this);
  LOG_TEXT_SENSOR("  ", "Version", this->version_);
  LOG_SWITCH("  ", "Buzzer", this->buzzer_);
  LOG_SWITCH("  ", "Led", this->led_);
  LOG_SENSOR("  ", "Outdoor Temperature", this->outdoor_temperature_);
  LOG_SENSOR("  ", "Heater Power", this->heater_power_);
  LOG_SENSOR("  ", "Airflow Counter", this->airflow_counter_);
  LOG_SENSOR("  ", "Filter Time Left", this->filter_time_left_);
  LOG_BINARY_SENSOR("  ", "Filter Warnout", this->filter_warnout_);
#ifdef TION_ENABLE_PRESETS
  LOG_NUMBER("  ", "Boost Time", this->boost_time_);
  LOG_SENSOR("  ", "Boost Time Left", this->boost_time_left_);
#endif  // TION_ENABLE_PRESETS
}

}  // namespace tion
}  // namespace esphome
