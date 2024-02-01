#include "esphome/core/defines.h"
#ifdef USE_CLIMATE
#ifdef TION_ENABLE_PRESETS
#include <cinttypes>
#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
#endif

#include "tion_climate_presets.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_presets";

// boost time update interval
#define BOOST_TIME_UPDATE_INTERVAL_SEC 20

// application scheduler name
static const char *const ASH_BOOST = "tion-boost";

void TionClimatePresets::setup_presets() {
  if (this->boost_time_) {
    this->boost_rtc_ = global_preferences->make_preference<uint8_t>(fnv1_hash("boost_time"));
    uint8_t boost_time;
    if (!this->boost_rtc_.load(&boost_time)) {
      boost_time = DEFAULT_BOOST_TIME_SEC / 60;
    }
    auto call = this->boost_time_->make_call();
    call.set_value(boost_time);
    call.perform();
    this->boost_time_->add_on_state_callback([this](float state) {
      const uint8_t boost_time = state;
      this->boost_rtc_.save(&boost_time);
    });
  }

#ifdef USE_API
  this->presets_rtc_ =
      global_preferences->make_preference<TionClimatePresetData[TION_MAX_PRESETS]>(fnv1_hash("presets"));
  if (this->presets_rtc_.load(&this->presets_)) {
    this->presets_[0].fan_speed = 0;  // reset initialization
    ESP_LOGD(TAG, "Presets loaded");
  }
  api::CustomAPIDevice api;
  api.register_service(&TionClimatePresets::update_preset_service_, "update_preset",
                       {"preset", "mode", "fan_speed", "target_temperature", "gate_position"});
#endif
}

void TionClimatePresets::add_presets(climate::ClimateTraits &traits) {
  traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  this->for_each_preset_([&traits](auto index) { traits.add_supported_preset(index); });
}

#ifdef USE_API
// esphome services allows only pass copy of strings
void TionClimatePresets::update_preset_service_(std::string preset_str, std::string mode_str, int32_t fan_speed,
                                                int32_t target_temperature, std::string gate_position_str) {
  climate::ClimatePreset preset = climate::ClimatePreset::CLIMATE_PRESET_NONE;
  if (preset_str.length() > 0) {
    auto preset_ch = std::tolower(preset_str[0]);
    if (preset_ch == 'h') {  // Home
      preset = climate::ClimatePreset::CLIMATE_PRESET_HOME;
    } else if (preset_ch == 'a') {  // Away/Activity
      if (preset_str.length() > 1) {
        preset_ch = std::tolower(preset_str[1]);
        if (preset_ch == 'w') {  // aWay
          preset = climate::ClimatePreset::CLIMATE_PRESET_AWAY;
        } else if (preset_ch == 'c') {  // aCtivity
          preset = climate::ClimatePreset::CLIMATE_PRESET_ACTIVITY;
        }
      }
    } else if (preset_ch == 'b') {  // Boost
      preset = climate::ClimatePreset::CLIMATE_PRESET_BOOST;
    } else if (preset_ch == 'c') {  // Comform
      preset = climate::ClimatePreset::CLIMATE_PRESET_COMFORT;
    } else if (preset_ch == 'e') {  // Eco
      preset = climate::ClimatePreset::CLIMATE_PRESET_ECO;
    } else if (preset_ch == 's') {  // Sleep
      preset = climate::ClimatePreset::CLIMATE_PRESET_SLEEP;
    }
  }

  auto mode = climate::ClimateMode::CLIMATE_MODE_AUTO;
  if (mode_str.length() > 0) {
    auto mode_ch = std::tolower(mode_str[0]);
    if (mode_ch == 'h') {  // Heat
      mode = climate::ClimateMode::CLIMATE_MODE_HEAT;
    } else if (mode_ch == 'f') {  // Fan_only
      mode = climate::ClimateMode::CLIMATE_MODE_FAN_ONLY;
    } else if (mode_ch == 'o') {  // Off
      mode = climate::ClimateMode::CLIMATE_MODE_OFF;
    }
  }

  TionGatePosition gate_position = TionGatePosition::NONE;
  if (gate_position_str.length() > 0) {
    auto gate_position_ch = std::tolower(gate_position_str[0]);
    if (gate_position_ch == 'o') {  // Outdoor
      gate_position = TionGatePosition::OUTDOOR;
    } else if (gate_position_ch == 'i') {  // Indoor
      gate_position = TionGatePosition::INDOOR;
    } else if (gate_position_ch == 'm') {  // Mixed
      gate_position = TionGatePosition::MIXED;
    }
  }

  if (this->update_preset(preset, mode, fan_speed, target_temperature, gate_position)) {
    if (this->presets_rtc_.save(&this->presets_)) {
      ESP_LOGCONFIG(TAG, "Preset was updated:");
      this->dump_preset_(TAG, preset);
    }
  } else {
    ESP_LOGW(TAG, "Preset %s was't updated", preset_str.c_str());
  }
}
#endif  // USE_API

void TionClimatePresets::dump_presets(const char *TAG) const {
  LOG_NUMBER("  ", "Boost Time", this->boost_time_);
  LOG_SENSOR("  ", "Boost Time Left", this->boost_time_left_);
  auto has_presets = false;
  this->for_each_preset_([&has_presets](auto index) { has_presets = true; });
  if (has_presets) {
    ESP_LOGCONFIG(TAG, "  Presets (fan_speed, target_temperature, mode, gate_position):");
    this->for_each_preset_([TAG, this](auto index) { this->dump_preset_(TAG, index); });
  }
}

void TionClimatePresets::dump_preset_(const char *tag, climate::ClimatePreset index) const {
  auto gate_position_to_string = [](TionGatePosition gp) -> const char * {
    switch (gp) {
      case TionGatePosition::NONE:
        return "none";
      case TionGatePosition::OUTDOOR:
        return "outdoor";
      case TionGatePosition::INDOOR:
        return "indoor";
      case TionGatePosition::MIXED:
        return "mixed";
      default:
        return "unknown";
    }
  };
  const auto &preset = this->presets_[index];
  const auto *preset_str = LOG_STR_ARG(climate::climate_preset_to_string(index));
  const auto *mode_str = LOG_STR_ARG(climate::climate_mode_to_string(preset.mode));
  const auto *gate_pos_str = gate_position_to_string(preset.gate_position);
#ifdef USE_ESP8266
  ESP_LOGCONFIG(tag, "    %-8s: %u, %2d, %-8s, %s", preset_str, preset.fan_speed, preset.target_temperature, mode_str,
                gate_pos_str);
#else
  ESP_LOGCONFIG(tag, "    %-8s: %u, %2d, %-8s, %s", str_lower_case(preset_str).c_str(), preset.fan_speed,
                preset.target_temperature, str_lower_case(mode_str).c_str(), gate_pos_str);
#endif
}

TionClimatePresetData *TionClimatePresets::presets_enable_preset_(climate::ClimatePreset new_preset,
                                                                  Component *component, climate::Climate *climate) {
  const auto old_preset = climate->preset.value_or(climate::CLIMATE_PRESET_NONE);
  if (new_preset == old_preset) {
    ESP_LOGD(TAG, "Preset was not changed");
    return nullptr;
  }

  if (new_preset >= TION_MAX_PRESETS) {
    ESP_LOGW(TAG, "Unknown preset number %u", new_preset);
    return nullptr;
  }

  if (old_preset == climate::CLIMATE_PRESET_BOOST) {
    ESP_LOGD(TAG, "Cancel preset boost");
    this->presets_cancel_boost_(component, climate);
  }

  ESP_LOGD(TAG, "Enable preset %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
  if (new_preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->presets_enable_boost_(component, climate)) {
      return nullptr;
    }
    this->saved_preset_ = old_preset;
    // инициализируем дефолный пресет NONE чтобы можно было в него восстановиться в любом случае
    if (!this->presets_[climate::CLIMATE_PRESET_NONE].is_initialized() && old_preset != climate::CLIMATE_PRESET_NONE) {
      this->update_default_preset_(climate);
    }
  }

  // если был пресет NONE, то сохраним его текущее состояние
  if (old_preset == climate::CLIMATE_PRESET_NONE) {
    this->update_default_preset_(climate);
  }

  // дополнительно проверим, что пресет был предварительно сохранен (см. блок выше)
  // в противном случае можем получить зимой, например, отстутсвие подогрева
  // т.е. неинициализированный пресет не активируем
  if (!this->presets_[new_preset].is_initialized()) {
    ESP_LOGW(TAG, "No data for preset %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
    return nullptr;
  }

  return &this->presets_[new_preset];
}

// TODO remove this method and use this->enable_preset_(this->saved_preset_);
TionClimatePresetData *TionClimatePresets::presets_cancel_preset_(climate::ClimatePreset old_preset,
                                                                  Component *component, climate::Climate *climate) {
  if (old_preset == climate::CLIMATE_PRESET_BOOST) {
    return this->presets_enable_preset_(this->saved_preset_, component, climate);
  }
  return nullptr;
}

void TionClimatePresets::update_default_preset_(climate::Climate *climate) {
  this->presets_[climate::CLIMATE_PRESET_NONE].mode = climate->mode;
  this->presets_[climate::CLIMATE_PRESET_NONE].target_temperature = climate->target_temperature;
  this->presets_[climate::CLIMATE_PRESET_NONE].gate_position = this->get_gate_position();
  const auto fan_speed = fan_mode_to_speed(climate->custom_fan_mode);
  if (fan_speed != 0) {
    this->presets_[climate::CLIMATE_PRESET_NONE].fan_speed = fan_speed;
  }
}

bool TionClimatePresets::presets_enable_boost_(Component *component, climate::Climate *climate) {
  auto boost_time = this->get_boost_time_();
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %" PRIu32 " s", boost_time);
    App.scheduler.set_timeout(component, ASH_BOOST, boost_time * 1000, [this, component, climate]() {
      this->presets_cancel_preset_(climate::CLIMATE_PRESET_BOOST, component, climate);
    });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %" PRIu32 " s", boost_time);
  this->boost_time_left_->publish_state(static_cast<float>(boost_time));

  App.scheduler.set_interval(component, ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this, component, climate]() {
    const int32_t time_left = static_cast<int32_t>(this->boost_time_left_->state) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    ESP_LOGV(TAG, "Boost time left %" PRId32 " s", time_left);
    if (time_left > 0) {
      this->boost_time_left_->publish_state(static_cast<float>(time_left));
    } else {
      this->presets_cancel_preset_(climate::CLIMATE_PRESET_BOOST, component, climate);
    }
  });

  return true;
}

void TionClimatePresets::presets_cancel_boost_(Component *component, climate::Climate *climate) {
  if (this->boost_time_left_) {
    ESP_LOGV(TAG, "Cancel boost interval");
    App.scheduler.cancel_interval(component, ASH_BOOST);
    this->boost_time_left_->publish_state(NAN);
  } else {
    ESP_LOGV(TAG, "Cancel boost timeout");
    App.scheduler.cancel_timeout(component, ASH_BOOST);
  }
}

}  // namespace tion
}  // namespace esphome
#endif  // TION_ENABLE_PRESETS
#endif  // USE_CLIMATE
