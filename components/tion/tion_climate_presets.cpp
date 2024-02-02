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
  if (this->presets_boost_time_) {
    this->presets_boost_rtc_ = global_preferences->make_preference<uint8_t>(fnv1_hash("boost_time"));
    uint8_t boost_time;
    if (!this->presets_boost_rtc_.load(&boost_time)) {
      boost_time = DEFAULT_BOOST_TIME_SEC / 60;
    }
    auto call = this->presets_boost_time_->make_call();
    call.set_value(boost_time);
    call.perform();
    this->presets_boost_time_->add_on_state_callback([this](float state) {
      const uint8_t boost_time = state;
      this->presets_boost_rtc_.save(&boost_time);
    });
  }

#ifdef USE_API
  this->presets_data_rtc_ =
      global_preferences->make_preference<TionClimatePresetData[TION_MAX_PRESETS]>(fnv1_hash("presets"));
  if (this->presets_data_rtc_.load(&this->presets_data_)) {
    this->presets_data_[0].fan_speed = 0;  // reset initialization
    ESP_LOGD(TAG, "Presets loaded");
  }
  api::CustomAPIDevice api;
  api.register_service(&TionClimatePresets::presets_update_service_, "update_preset",
                       {"preset", "mode", "fan_speed", "target_temperature", "gate_position"});
#endif
}

void TionClimatePresets::add_presets(climate::ClimateTraits &traits) {
  traits.add_supported_preset(climate::CLIMATE_PRESET_NONE);
  this->presets_for_each_([&traits](auto index) { traits.add_supported_preset(index); });
}

#ifdef USE_API
// esphome services allows only pass copy of strings
void TionClimatePresets::presets_update_service_(std::string preset_str, std::string mode_str, int32_t fan_speed,
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
    if (this->presets_data_rtc_.save(&this->presets_data_)) {
      ESP_LOGCONFIG(TAG, "Preset was updated:");
      this->presets_dump_preset_(TAG, preset);
    }
  } else {
    ESP_LOGW(TAG, "Preset %s was't updated", preset_str.c_str());
  }
}
#endif  // USE_API

void TionClimatePresets::dump_presets(const char *TAG) const {
  LOG_NUMBER("  ", "Boost Time", this->presets_boost_time_);
  LOG_SENSOR("  ", "Boost Time Left", this->presets_boost_time_left_);
  if (this->has_presets()) {
    ESP_LOGCONFIG(TAG, "  Presets (fan_speed, target_temperature, mode, gate_position):");
    this->presets_for_each_([TAG, this](auto index) { this->presets_dump_preset_(TAG, index); });
  }
}

void TionClimatePresets::presets_dump_preset_(const char *tag, climate::ClimatePreset index) const {
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
  const auto &preset_data = this->presets_data_[index];
  const auto *preset_str = LOG_STR_ARG(climate::climate_preset_to_string(index));
  const auto *mode_str = LOG_STR_ARG(climate::climate_mode_to_string(preset_data.mode));
  const auto *gate_pos_str = gate_position_to_string(preset_data.gate_position);
  ESP_LOGCONFIG(tag, "    %-8s: %u, %2d, %-8s, %s", preset_str, preset_data.fan_speed, preset_data.target_temperature,
                mode_str, gate_pos_str);
}

TionClimatePresetData *TionClimatePresets::presets_activate_preset_(climate::ClimatePreset new_preset,
                                                                    Component *component, climate::Climate *climate) {
  if (new_preset >= TION_MAX_PRESETS) {
    ESP_LOGW(TAG, "Unknown preset number %u", new_preset);
    return nullptr;
  }

  const auto old_preset = climate->preset.value_or(climate::CLIMATE_PRESET_NONE);

  const auto *new_preset_name = LOG_STR_ARG(climate::climate_preset_to_string(new_preset));

  if (new_preset == old_preset) {
    ESP_LOGD(TAG, "Preset %s was not changed", new_preset_name);
    return nullptr;
  }

  // Проверим включен ли пресет для активации. Пресет NONE всегда включен, пропустим его.
  if (new_preset != climate::CLIMATE_PRESET_NONE && !this->presets_data_[new_preset].is_enabled()) {
    ESP_LOGW(TAG, "Preset %s is disabled", new_preset_name);
    return nullptr;
  }

  if (old_preset == climate::CLIMATE_PRESET_BOOST) {
    if (this->presets_boost_time_left_ == nullptr ||
        (this->presets_boost_time_left_ && !std::isnan(this->presets_boost_time_left_->state))) {
      ESP_LOGD(TAG, "Cancel boost preset");
    }
    this->presets_cancel_boost_timer_(component);
  }

  ESP_LOGD(TAG, "Activate preset %s", new_preset_name);
  if (new_preset == climate::CLIMATE_PRESET_BOOST) {
    if (!this->presets_enable_boost_timer_(component, climate)) {
      return nullptr;
    }
    this->presets_saved_preset_ = old_preset;
    // инициализируем дефолный пресет NONE чтобы можно было в него восстановиться в любом случае
    if (!this->presets_data_[climate::CLIMATE_PRESET_NONE].is_initialized() &&
        old_preset != climate::CLIMATE_PRESET_NONE) {
      this->presets_save_default_(climate);
    }
  }

  // если был пресет NONE, то сохраним его текущее состояние
  if (old_preset == climate::CLIMATE_PRESET_NONE) {
    this->presets_save_default_(climate);
  }

  // дополнительно проверим, что пресет был предварительно сохранен (см. блок выше)
  // в противном случае можем получить зимой, например, отстутсвие подогрева
  // т.е. неинициализированный пресет не активируем
  if (!this->presets_data_[new_preset].is_initialized()) {
    ESP_LOGW(TAG, "Preset %s is not initialized", new_preset_name);
    return nullptr;
  }

  ESP_LOGV(TAG, "Preset data:");
  ESP_LOGV(TAG, "  mode: %s", LOG_STR_ARG(climate::climate_mode_to_string(this->presets_data_[new_preset].mode)));
  ESP_LOGV(TAG, "  fan_speed: %u", this->presets_data_[new_preset].fan_speed);
  ESP_LOGV(TAG, "  target_temperature: %d", this->presets_data_[new_preset].target_temperature);
  ESP_LOGV(TAG, "  gate_position: %u", static_cast<uint8_t>(this->get_gate_position()));

  return &this->presets_data_[new_preset];
}

void TionClimatePresets::presets_save_default_(climate::Climate *climate) {
  this->presets_data_[climate::CLIMATE_PRESET_NONE].mode = climate->mode;
  this->presets_data_[climate::CLIMATE_PRESET_NONE].target_temperature = climate->target_temperature;
  this->presets_data_[climate::CLIMATE_PRESET_NONE].gate_position = this->get_gate_position();
  const auto fan_speed = fan_mode_to_speed(climate->custom_fan_mode);
  // не сохраняем fan_speed=0, т.к. это маркер инициализации,
  // а выключенное состояние в люом случае задается режимом.
  if (fan_speed != 0) {
    this->presets_data_[climate::CLIMATE_PRESET_NONE].fan_speed = fan_speed;
  }
}

bool TionClimatePresets::presets_enable_boost_timer_(Component *component, climate::Climate *climate) {
  auto boost_time = this->get_boost_time_();
  if (boost_time == 0) {
    ESP_LOGW(TAG, "Boost time is not configured");
    return false;
  }

  // if boost_time_left not configured, just schedule stop boost after boost_time
  if (this->presets_boost_time_left_ == nullptr) {
    ESP_LOGD(TAG, "Schedule boost timeout for %" PRIu32 " s", boost_time);
    App.scheduler.set_timeout(component, ASH_BOOST, boost_time * 1000,
                              [this, climate]() { this->presets_cancel_boost_(climate); });
    return true;
  }

  // if boost_time_left is configured, schedule update it
  ESP_LOGD(TAG, "Schedule boost interval up to %" PRIu32 " s", boost_time);
  this->presets_boost_time_left_->publish_state(static_cast<float>(boost_time));

  App.scheduler.set_interval(component, ASH_BOOST, BOOST_TIME_UPDATE_INTERVAL_SEC * 1000, [this, climate]() {
    const int32_t time_left =
        static_cast<int32_t>(this->presets_boost_time_left_->state) - BOOST_TIME_UPDATE_INTERVAL_SEC;
    ESP_LOGV(TAG, "Boost time left %" PRId32 " s", time_left);
    if (time_left > 0) {
      this->presets_boost_time_left_->publish_state(static_cast<float>(time_left));
    } else {
      this->presets_boost_time_left_->publish_state(NAN);
      this->presets_cancel_boost_(climate);
    }
  });

  return true;
}

void TionClimatePresets::presets_cancel_boost_timer_(Component *component) {
  if (this->presets_boost_time_left_ == nullptr) {
    ESP_LOGV(TAG, "Cancel boost timeout");
    App.scheduler.cancel_timeout(component, ASH_BOOST);
  } else {
    ESP_LOGV(TAG, "Cancel boost interval");
    App.scheduler.cancel_interval(component, ASH_BOOST);
    this->presets_boost_time_left_->publish_state(NAN);
  }
}

void TionClimatePresets::presets_cancel_boost_(climate::Climate *climate) {
  const auto new_preset = this->presets_saved_preset_;
  ESP_LOGD(TAG, "Boost finished, switch back to %s", LOG_STR_ARG(climate::climate_preset_to_string(new_preset)));
  auto call = climate->make_call();
  call.set_preset(new_preset);
  call.perform();
}

}  // namespace tion
}  // namespace esphome
#endif  // TION_ENABLE_PRESETS
#endif  // USE_CLIMATE
