#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include "esphome/components/climate/climate.h"

#include "../tion_component.h"
#include "../tion_extension.h"

namespace esphome {
namespace tion {

class TionClimate : public EntityExtension<climate::Climate>, public Component, public Parented<TionApiComponent> {
  using TionState = dentra::tion::TionState;

 public:
  explicit TionClimate(TionApiComponent *api) : Parented(api) {}

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void dump_config() override;
  void setup() override;

  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void set_enable_heat_cool(bool enable_heat_cool) { this->options_.enable_heat_cool = enable_heat_cool; }
  void set_enable_fan_auto(bool enable_fan_auto) { this->options_.enable_fan_auto = enable_fan_auto; }
  void set_enable_fan_off(bool enable_fan_off) { this->options_.enable_fan_off = enable_fan_off; }
  // экспонирует air_intake (заслонку) как preset_mode climate-сущности вместо/в дополнение к select.*_air_intake;
  // несовместимо с реальными пресетами устройства (has_presets()) — при их наличии игнорируется
  void set_enable_air_intake_preset(bool enable_air_intake_preset) {
    this->options_.enable_air_intake_preset = enable_air_intake_preset;
  }

 protected:
  struct {
    bool enable_heat_cool : 1;
    bool enable_fan_auto : 1;
    bool enable_fan_off : 1;
    bool enable_air_intake_preset : 1;
  } options_{};
  void on_state_(const TionState &state);
  // важно this->mode уже должен быть выставлен в актуальное значение
  bool set_fan_speed_(const TionState &state);
};

}  // namespace tion
}  // namespace esphome
