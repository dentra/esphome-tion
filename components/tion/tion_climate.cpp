#include "esphome/core/defines.h"
#ifdef USE_CLIMATE
#include "esphome/core/log.h"
#include "tion_climate.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_climate";

climate::ClimateTraits TionClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(1.0f);
  traits.set_visual_max_temperature(TION_MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1.0f);
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
#ifdef USE_TION_CLIMATE_MODE_HEAT_COOL
      climate::CLIMATE_MODE_HEAT_COOL,
#endif
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  for (uint8_t i = 1, max = i + this->max_fan_speed_; i < max; i++) {
    traits.add_supported_custom_fan_mode(this->fan_speed_to_mode_(i));
  }
#ifdef TION_ENABLE_PRESETS
  if (!this->supported_presets_.empty()) {
    traits.set_supported_presets(this->supported_presets_);
    traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  }
#endif
  traits.set_supports_action(true);
  return traits;
}

void TionClimate::control(const climate::ClimateCall &call) {
#ifdef TION_ENABLE_PRESETS
  if (call.get_preset().has_value()) {
    const auto new_preset = *call.get_preset();
    const auto old_preset = *this->preset;
    if (new_preset != old_preset) {
      this->cancel_preset_(old_preset);
      if (this->enable_preset_(new_preset)) {
        return;
      }
    }
  }
#endif

  climate::ClimateMode mode = this->mode;
  if (call.get_mode().has_value()) {
    mode = *call.get_mode();
    ESP_LOGD(TAG, "Set mode %s", LOG_STR_ARG(climate::climate_mode_to_string(mode)));
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  uint8_t fan_speed = this->fan_mode_to_speed_(this->custom_fan_mode);
  if (call.get_custom_fan_mode().has_value()) {
    fan_speed = this->fan_mode_to_speed_(call.get_custom_fan_mode());
    ESP_LOGD(TAG, "Set fan speed %u", fan_speed);
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  int8_t target_temperature = this->target_temperature;
  if (call.get_target_temperature().has_value()) {
    target_temperature = *call.get_target_temperature();
    ESP_LOGD(TAG, "Set target temperature %d °C", target_temperature);
    this->preset = climate::CLIMATE_PRESET_NONE;
  }

  this->control_climate_state(mode, fan_speed, target_temperature);
}

void TionClimate::set_fan_speed_(uint8_t fan_speed) {
  if (fan_speed > 0 && fan_speed <= this->max_fan_speed_) {
    this->custom_fan_mode = this->fan_speed_to_mode_(fan_speed);
  } else {
    if (!(this->mode == climate::CLIMATE_MODE_OFF && fan_speed == 0)) {
      ESP_LOGW(TAG, "Unsupported fan speed %u (max: %u)", fan_speed, this->max_fan_speed_);
    }
  }
}

void TionClimate::dump_presets(const char *tag) const {
#ifdef TION_ENABLE_PRESETS
  ESP_LOGCONFIG(tag, "  Presets:");
  for (size_t i = 1; i < sizeof(this->presets_) / sizeof(this->presets_[0]); i++) {
    ESP_LOGCONFIG(tag, "    %s: fan speed=%u, target temperature=%d, mode=%s",
                  LOG_STR_ARG(climate::climate_preset_to_string(static_cast<climate::ClimatePreset>(i))),
                  this->presets_[i].fan_speed, this->presets_[i].target_temperature,
                  LOG_STR_ARG(climate::climate_mode_to_string(this->presets_[i].mode)));
  }
#endif
}

#ifdef TION_ENABLE_PRESETS

bool TionClimate::enable_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->enable_boost_()) {
      ESP_LOGW(TAG, "Boost time is not configured");
      return false;
    }
    this->saved_preset_ = *this->preset;
  }

  // если был пресет NONE, то сохраним его значения
  if (*this->preset == climate::CLIMATE_PRESET_NONE) {
    this->presets_[climate::CLIMATE_PRESET_NONE].mode = this->mode;
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = this->get_fan_speed_();
    this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = this->target_temperature;
  }

  // дополнительно проверим, что пресет был предварительно сохранен (см. блок выше)
  // в противном случае можем получить зимой отстутсвие подогрева
  // т.е. неинициализированный NONE пресет не активируем
  if (this->presets_[preset].fan_speed != 0) {
    this->control_climate_state(this->presets_[preset].mode, this->presets_[preset].fan_speed,
                                this->presets_[preset].target_temperature);
  }
  this->preset = preset;

  return true;
}

void TionClimate::cancel_preset_(climate::ClimatePreset preset) {
  ESP_LOGD(TAG, "Cancel preset %s", LOG_STR_ARG(climate::climate_preset_to_string(preset)));
  if (preset == climate::CLIMATE_PRESET_BOOST) {
    this->cancel_boost_();
    this->enable_preset_(this->saved_preset_);
  }
}
#endif

}  // namespace tion
}  // namespace esphome
#endif
