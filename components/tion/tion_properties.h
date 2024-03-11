#pragma once

#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include "../tion-api/tion-api.h"
#include "tion_api_component.h"

namespace esphome {
namespace tion {
namespace property_controller {

using dentra::tion::TionState;
using dentra::tion::TionTraits;
using dentra::tion::TionGatePosition;
using dentra::tion::TionStateCall;

template<typename C> class Controller {
  constexpr static const auto *TAG = "tion_properties";

  class Checker {
    constexpr static auto TAC = static_cast<TionApiComponent *>(nullptr);
    constexpr static auto TSC = static_cast<TionStateCall *>(nullptr);

    template<typename T>
    auto test_is_supported(TionApiComponent *api)
        -> std::enable_if_t<sizeof(decltype(T::is_supported(api)) *) != 0, std::true_type>;
    template<typename T> std::false_type test_is_supported(...);

    template<typename T>
    auto test_api_state_set(TionApiComponent *api, TionStateCall *call)
        -> std::enable_if_t<sizeof(decltype(T::set(api, call, 0)) *) != 0, std::true_type>;
    template<typename T> std::false_type test_api_state_set(...);

    template<typename T>
    auto test_state_set(TionStateCall *call)
        -> std::enable_if_t<sizeof(decltype(T::set(call, 0)) *) != 0, std::true_type>;
    template<typename T> std::false_type test_state_set(...);

    template<typename T>
    auto test_api_get(TionApiComponent *api) -> std::enable_if_t<sizeof(decltype(T::get(api)) *) != 0, std::true_type>;
    template<typename T> std::false_type test_api_get(...);

    template<typename T>
    auto test_api_state_get(TionApiComponent *api)
        -> std::enable_if_t<sizeof(decltype(T::get(api, {})) *) != 0, std::true_type>;
    template<typename T> std::false_type test_api_state_get(...);

    template<typename T>
    auto test_icon_get(TionApiComponent *api)
        -> std::enable_if_t<sizeof(decltype(T::get_icon(api)) *) != 0, std::true_type>;
    template<typename T> std::false_type test_icon_get(...);

   public:
    constexpr bool has_is_supported() { return decltype(test_is_supported<C>(TAC))::value; }

    constexpr bool has_api_state_get() { return decltype(test_api_state_get<C>(TAC))::value; }
    constexpr bool has_api_get() { return decltype(test_api_get<C>(TAC))::value; }

    constexpr bool has_api_state_set() { return decltype(test_api_state_set<C>(TAC, TSC))::value; }
    constexpr bool has_state_set() { return decltype(test_state_set<C>(TSC))::value; }
    constexpr bool has_api_set() { return !has_state_set() && !has_api_state_set(); }

    constexpr bool has_icon_get() { return decltype(test_icon_get<C>(TAC))::value; }
  };

 public:
  static constexpr Checker checker() { return Checker{}; }

  template<typename T> static bool is_supported(T *component [[maybe_unused]]) {
    if constexpr (checker().has_is_supported()) {
      if (!C::is_supported(component->get_parent())) {
        mark_unsupported(component);
        return false;
      }
    }
    return true;
  }

  template<typename T> static void mark_unsupported_entity(T *entity) {
    report_unsupported(entity);
    entity->set_icon("mdi:help");
    entity->set_internal(true);
  }

  template<typename T> static void mark_unsupported(T *component) {
    mark_unsupported_entity(component);
    component->mark_failed();
  }

  template<typename T> static void report_unsupported(T *component) {
    ESP_LOGW(TAG, "Unsupported %s", component->get_name().c_str());
  }

  template<typename T> static bool publish_state(T *component, const TionState *state) {
    // данные об изменении иконки не бнволяются в рантайме
    // if constexpr (checker().has_icon_get()) {
    //   component->set_icon(C::get_icon(component->get_parent()));
    // }

    if constexpr (checker().has_api_get()) {
      // здесь обработчик не зависит от статуса
      auto st = C::get(component->get_parent());
      if (component->get_parent()->get_force_update() || !component->has_state() || st != component->state) {
        component->publish_state(st);
      }
      return true;
    } else {
      if (state == nullptr) {
        // пустой state означает, ошибку получения состоянияя
        return false;
      }
      if constexpr (checker().has_api_state_get()) {
        auto st = C::get(component->get_parent(), *state);
        if (component->get_parent()->get_force_update() || !component->has_state() || st != component->state) {
          component->publish_state(st);
        }
      } else {
        auto st = C::get(*state);
        if (component->get_parent()->get_force_update() || !component->has_state() || st != component->state) {
          component->publish_state(st);
        }
      }
      return true;
    }
  }

  template<typename T, typename V> static void control(T *component, V state) {
    if (component->is_failed()) {
      report_unsupported(component);
      return;
    }
    if constexpr (checker().has_state_set() || checker().has_api_state_set()) {
      auto *call = component->parent_->make_call();
      if (call == nullptr) {
        ESP_LOGW(TAG, "Make call failed for %s", component->get_name().c_str());
        return;
      }
      if constexpr (checker().has_api_state_set()) {
        C::set(component->parent_, call, state);
      } else {
        C::set(call, state);
      }
#ifdef TION_DUMP
      call->dump();
#endif
      call->perform();
    } else {
      C::set(component->parent_, state);
    }
  }
};

namespace binary_sensor {
struct Power {
  static const char *get_icon(TionApiComponent *api) {
    return api->state().power_state ? "mdi:power" : "mdi:power-off";
  }

  static bool get(const TionState &state) { return state.power_state; }
};

struct Heater {
  static const char *get_icon(TionApiComponent *api) {
    return api->state().heater_state ? "mdi:radiator" : "mdi:radiator-off";
  }

  static bool get(const TionState &state) { return state.heater_state; }
};

struct Sound {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_sound_state; }

  static const char *get_icon(TionApiComponent *api) {
    return api->state().sound_state ? "mdi:volume-high" : "mdi:volume-mute";
  }

  static bool get(const TionState &state) { return state.sound_state; }
};

struct Led {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_led_state; }

  static const char *get_icon(TionApiComponent *api) { return api->state().led_state ? "mdi:led-on" : "mdi:led-off"; }

  static bool get(const TionState &state) { return state.led_state; }
};

struct Auto {
  static bool get(const TionState &state) { return state.auto_state; }
};

struct Filter {
  static bool get(const TionState &state) { return state.filter_state; }
};

struct GateError {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_gate_error; }

  static bool get(const TionState &state) { return state.gate_error_state; }
};

struct GatePosition {
  static const char *get_icon(TionApiComponent *api) {
    if (api->traits().supports_gate_position_change_mixed && api->state().gate_position == TionGatePosition::MIXED) {
      return "mdi:valve";
    }
    return api->state().get_gate_state() ? "mdi:valve-open" : "mdi:valve-closed";
  }

  static bool get(const TionState &state) { return state.get_gate_state(); }
};

struct Heating {
  static const char *get_icon(TionApiComponent *api) { return Heater::get_icon(api); }

  static bool get(TionApiComponent *api, const TionState &state) { return state.is_heating(api->traits()); }
};

struct Error {
  static bool get(const TionState &state) { return state.errors != 0; }
};

struct Boost {
  static bool get(const TionState &state) { return state.boost_time_left > 0; }
};

struct State {
  static bool get(TionApiComponent *api) { return !api->has_state(); }
};

}  // namespace binary_sensor

namespace switch_ {
struct Power : public binary_sensor::Power {
  static void set(TionApiComponent *api, TionStateCall *call, bool state) { call->set_power_state(state); }
};

struct Heater : public binary_sensor::Heater {
  static void set(TionStateCall *call, bool state) { call->set_heater_state(state); }
};

struct Sound : public binary_sensor::Sound {
  static void set(TionStateCall *call, bool state) { call->set_sound_state(state); }
};

struct Led : public binary_sensor::Led {
  static void set(TionStateCall *call, bool state) { call->set_led_state(state); }
};

struct Auto : public binary_sensor::Auto {
  static void set(TionStateCall *call, bool state) { call->set_auto_state(state); }
};

struct Recirculation {
  static bool is_supported(TionApiComponent *api) {
    return api->traits().supports_gate_position_change || api->traits().supports_gate_position_change_mixed;
  }

  static const char *get_icon(TionApiComponent *api) { return binary_sensor::GatePosition::get_icon(api); }

  static bool get(const TionState &state) { return binary_sensor::GatePosition::get(state); }
  static void set(TionStateCall *call, bool state) { call->set_gate_state(state); }
};

struct Boost : public binary_sensor::Boost {
  static void set(TionApiComponent *api, bool state) { api->api()->enable_boost(state, api->make_call()); }
};

}  // namespace switch_

namespace sensor {
struct FanSpeed {
  static const char *get_icon(TionApiComponent *api) {
    if (!api->state().power_state) {
      return "mdi:fan-off";
    }
    if (api->state().boost_time_left > 0) {
      return "mdi:fan-clock";
    }
    if (api->state().auto_state) {
      return "mdi:fan-auto";
    }
    if (api->state().fan_speed == 1) {
      return "mdi:fan-speed-1";
    }
    if (api->state().fan_speed == 2) {
      return "mdi:fan-speed-2";
    }
    if (api->state().fan_speed == 3) {
      return "mdi:fan-speed-3";
    }
    return "mdi:fan";
  }

  static uint8_t get(const TionState &state) { return state.power_state ? state.fan_speed : 0; }
};

// struct GatePosition {
//   static const char *get_icon(TionApiComponent *api) { return binary_sensor::GatePosition::get_icon(api); }
//   static uint8_t get(const TionState &state) { return static_cast<uint8_t>(state.gate_position); }
// };

struct OutdoorTemperature {
  static int8_t get(const TionState &state) { return state.outdoor_temperature; }
};

struct CurrentTemperature {
  static int8_t get(const TionState &state) { return state.current_temperature; }
};

struct TargetTemperature {
  static constexpr int8_t get(const TionState &state) { return state.target_temperature; }
};

struct Productivity {
  static uint8_t get(const TionState &state) { return state.productivity; }
};

struct HeaterVar {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_heater_var; }

  static uint8_t get(const TionState &state) { return state.heater_var; }
};

struct HeaterPower {
  static float get(TionApiComponent *api, const TionState &state) { return state.get_heater_power(api->traits()); }
};

struct WorkTime {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_work_time; }

  static uint32_t get(const TionState &state) { return state.work_time; }
};

struct FilterTimeLeft {
  static uint32_t get(const TionState &state) { return state.filter_time_left; }
};

struct FanTime {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_fan_time; }

  static uint32_t get(const TionState &state) { return state.fan_time; }
};

struct Airflow {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_airflow_counter; }

  static float get(const TionState &state) { return state.airflow_m3; }
};

struct AirflowCounter {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_airflow_counter; }

  static uint32_t get(const TionState &state) { return state.airflow_counter; }
};

struct PcbCtlTemperature {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_pcb_ctl_temperatire; }

  static int8_t get(const TionState &state) { return state.pcb_ctl_temperature; }
};

struct PcbPwrTemperature {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_pcb_pwr_temperature; }

  static int8_t get(const TionState &state) { return state.pcb_pwr_temperature; }
};

struct BoostTimeLeft {
  static float get(const TionState &state) { return state.boost_time_left > 0 ? state.boost_time_left : 0; }
};

struct FanPower {
  static float get(TionApiComponent *api, const TionState &state) {
    return api->traits().max_fan_power[state.power_state ? state.fan_speed : 0] / 100.0f;
  }
};

}  // namespace sensor

namespace number {

struct FanSpeed : public sensor::FanSpeed {
  static void set(TionApiComponent *api, TionStateCall *call, uint8_t state) {
    if (state) {
      if (state != api->state().fan_speed) {
        call->set_power_state(true);
        call->set_fan_speed(state);
      }
    } else {
      if (api->state().power_state) {
        call->set_power_state(false);
      }
    }
  }

  static constexpr uint8_t get_min(TionApiComponent *api) { return 0; }
  static uint8_t get_max(TionApiComponent *api) { return api->traits().max_fan_speed; }
};

// struct GatePosition : public sensor::GatePosition {
//   static void set(TionStateCall *call, uint8_t state) {
//   call->set_gate_position(static_cast<TionGatePosition>(state)); } static uint8_t get_min(TionApiComponent *api) {
//   return 0; } static uint8_t get_max(TionApiComponent *api) {
//     if (api->traits().supports_gate_position_change_mixed) {
//       return 3;
//     }
//     if (api->traits().supports_gate_position_change) {
//       return 2;
//     }
//     return 0;
//   }
// };

struct TargetTemperature : public sensor::TargetTemperature {
  static void set(TionStateCall *call, int8_t state) { call->set_target_temperature(state); }

  static int8_t get_min(TionApiComponent *api) { return api->traits().min_target_temperature; }
  static int8_t get_max(TionApiComponent *api) { return api->traits().max_target_temperature; }
};

struct BoostTime {
  static int8_t get(TionApiComponent *api) { return api->traits().boost_time / 60; }
  static void set(TionApiComponent *api, int8_t state) { api->api()->set_boost_time(state * 60); }

  static constexpr int8_t get_min(TionApiComponent *api) { return 1; }
  static constexpr int8_t get_max(TionApiComponent *api) { return 60; }
};

}  // namespace number

namespace text_sensor {

struct Errors {
  static std::string get(TionApiComponent *api, const TionState &state) {
    return api->traits().errors_decoder(state.errors);
  };
};

// struct GatePosition {
//   static const char *get_icon(TionApiComponent *api) { return binary_sensor::GatePosition::get_icon(api); }
//   static std::string get(TionApiComponent *api, const TionState &state) {
//     return state.get_gate_position_str(api->traits());
//   }
// };

struct FirmwareVersion {
  static std::string get(const TionState &state) {
    if (!state.firmware_version) {
      return {};
    }
    return str_snprintf("%04X", 4, state.firmware_version);
  }
};

struct HardwareVersion {
  static std::string get(const TionState &state) {
    if (!state.hardware_version) {
      return {};
    }
    return str_snprintf("%04X", 4, state.hardware_version);
  }
};

}  // namespace text_sensor

namespace select {

struct AirIntake {
  static std::vector<std::string> get_options(TionApiComponent *api) {
    if (api->traits().supports_gate_position_change_mixed) {
      return {"outdoor", "indoor", "mixed"};
    }
    if (api->traits().supports_gate_position_change) {
      return {"inflow", "recirculation"};
    }
    return {};
  };
  static std::string get(const TionState &state, const std::vector<std::string> &options) {
    const auto value = static_cast<uint8_t>(state.gate_position);
    if (value < options.size()) {
      return options[value];
    }
    return {};
  }
  static void set(TionStateCall *call, const std::string &state, const std::vector<std::string> &options) {
    for (size_t i = 0; i < options.size(); i++) {
      if (options[i] == state) {
        call->set_gate_position(static_cast<TionGatePosition>(i));
        break;
      }
    }
  }
};

struct Presets {
  static std::vector<std::string> get_options(TionApiComponent *api) { return api->api()->get_presets(); };
  static std::string get(TionApiComponent *api) { return api->api()->get_active_preset(); }
  static void set(TionApiComponent *api, TionStateCall *call, const std::string &preset) {
    api->api()->enable_preset(preset, call);
  }
};

}  // namespace select

namespace button {

struct ResetFilter {
  static bool is_supported(TionApiComponent *api) { return api->traits().supports_reset_filter; }

  static void press_action(TionApiComponent *api) { api->api()->reset_filter(); }
};

}  // namespace button

}  // namespace property_controller
}  // namespace tion
}  // namespace esphome
