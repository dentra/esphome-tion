#include <inttypes.h>

#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include "tion_4s.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_4s";

climate::ClimateTraits Tion4s::traits() {
  auto traits = TionClimate::traits();
  traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_BOOST});
  return traits;
}

void Tion4s::control(const climate::ClimateCall &call) {
  if (call.get_preset().has_value()) {
    // TODO update turbo state
    this->update_flag_ |= UPDATE_BOOST;
  }
  TionClimate::control(call);
}

void Tion4s::on_ready() { this->request_dev_status(); }

void Tion4s::read(const tion_dev_status_t &status) {
  if (status.device_type != tion_dev_status_t::BR4S) {
    this->parent_->set_enabled(false);
    ESP_LOGE(TAG, "Unsupported device type %04X", status.device_type);
    return;
  }
  this->read_dev_status_(status);

  // this->request_errors();
  // this->request_test();
  this->request_time();
  this->request_turbo();
  // this->request_timers();

  this->request_state();
};

void Tion4s::read(const tion4s_time_t &time) {
  auto tm = time::ESPTime::from_epoch_utc(time.unix_time);
  ESP_LOGD(TAG, "Device time %s", tm.strftime("%F %T").c_str());
}

void Tion4s::read(const tion4s_turbo_t &turbo) {
  if ((this->update_flag_ & UPDATE_BOOST) != 0) {
    this->update_flag_ &= ~UPDATE_BOOST;
    uint16_t turbo_time = this->boost_time_ ? this->boost_time_->state : turbo.turbo_time;
    if (turbo_time > 0) {
      TionApi4s::set_turbo_time(turbo_time);
    }
    return;
  }
  this->preset = turbo.is_active ? climate::CLIMATE_PRESET_BOOST : climate::CLIMATE_PRESET_NONE;
  if (this->boost_time_) {
    this->boost_time_->publish_state(turbo.turbo_time);
  }
  this->publish_state();
}

void Tion4s::read(const tion4s_state_t &state) {
  if ((this->update_flag_ & UPDATE_STATE) != 0) {
    this->update_flag_ &= ~UPDATE_STATE;
    tion4s_state_t st = state;
    this->update_state_(st);
    TionApi4s::write_state(st);
    return;
  }

  this->max_fan_speed_ = state.limits.max_fan_speed;

  if (state.system.power_state) {
    this->mode = state.system.heater_mode == tion4s_state_t::HEATER_MODE_HEATING ? climate::CLIMATE_MODE_HEAT
                                                                                 : climate::CLIMATE_MODE_FAN_ONLY;
  } else {
    this->mode = climate::CLIMATE_MODE_OFF;
  }

  this->target_temperature = state.system.target_temperature;

  this->set_fan_mode_(state.system.fan_speed);

  if (this->buzzer_) {
    this->buzzer_->publish_state(state.system.sound_state);
  } else {
    ESP_LOGI(TAG, "sound_state                : %s", ONOFF(state.system.sound_state));
  }
  if (this->led_) {
    this->led_->publish_state(state.system.led_state);
  } else {
    ESP_LOGI(TAG, "led_state                  : %s", ONOFF(state.system.led_state));
  }
  if (this->temp_in_) {
    this->temp_in_->publish_state(state.sensors.indoor_temperature);
  } else {
    ESP_LOGI(TAG, "indoor_temp    : %d", state.sensors.indoor_temperature);
  }
  if (this->temp_out_) {
    this->temp_out_->publish_state(state.sensors.outdoor_temperature);
  } else {
    ESP_LOGI(TAG, "outdoor_temp   : %d", state.sensors.outdoor_temperature);
  }
  if (this->heater_power_) {
    this->heater_power_->publish_state(state.heater_power());
  } else {
    ESP_LOGI(TAG, "heater_power   : %f", state.heater_power());
  }
  if (this->airflow_counter_) {
    this->airflow_counter_->publish_state(state.counters.airflow_counter());
  } else {
    ESP_LOGI(TAG, "airflow_counter: %f", state.counters.airflow_counter());
  }
  if (this->filter_warnout_) {
    this->filter_warnout_->publish_state(state.system.filter_wornout);
  } else {
    ESP_LOGI(TAG, "filter_wornout : %s", ONOFF(state.system.filter_wornout));
  }
  if (this->filter_days_left_) {
    this->filter_days_left_->publish_state(state.counters.fileter_days());
  } else {
    ESP_LOGI(TAG, "filter_time    : %u", state.counters.filter_time);
  }
  if (this->recirculation_) {
    this->recirculation_->publish_state(state.system.substate == tion4s_state_t::SUBSTATE_RECIRCULATION);
  } else {
    ESP_LOGI(TAG, "substate       : %u", state.system.substate);
  }

  ESP_LOGI(TAG, "pcb_pwr_temp   : %d", state.sensors.pcb_pwr_temperature);
  ESP_LOGI(TAG, "pcb_ctl_temp   : %d", state.sensors.pcb_ctl_temperature);
  ESP_LOGI(TAG, "fan_speed      : %u", state.system.fan_speed);
  ESP_LOGI(TAG, "heater_mode    : %u", state.system.heater_mode);
  ESP_LOGI(TAG, "heater_state   : %s", ONOFF(state.system.heater_state));
  ESP_LOGI(TAG, "heater_present : %u", state.system.heater_present);
  ESP_LOGI(TAG, "heater_var     : %u", state.heater_var);
  ESP_LOGI(TAG, "last_com_source: %u", state.system.last_com_source);

  ESP_LOGI(TAG, "ma             : %s", ONOFF(state.system.ma));
  ESP_LOGI(TAG, "ma_auto        : %s", ONOFF(state.system.ma_auto));
  ESP_LOGI(TAG, "active_timer   : %s", ONOFF(state.system.active_timer));
  ESP_LOGI(TAG, "reserved       : %02x", state.system.reserved);
  ESP_LOGI(TAG, "work_time      : %u", state.counters.work_time);
  ESP_LOGI(TAG, "fan_time       : %u", state.counters.fan_time);
  ESP_LOGI(TAG, "errors         : %u", state.errors);

  this->publish_state();

  // leave 3 sec connection left for end all of jobs
  App.scheduler.set_timeout(this, TAG, 3000, [this]() { this->parent_->set_enabled(false); });
}

void Tion4s::update_state_(tion4s_state_t &state) const {
  state.system.power_state = this->mode != climate::CLIMATE_MODE_OFF;
  state.system.heater_mode = this->mode == climate::CLIMATE_MODE_HEAT
                                 ? tion4s_state_t::HEATER_MODE_HEATING
                                 : tion4s_state_t::HEATER_MODE_TEMPERATURE_MAINTENANCE;
  if (this->recirculation_) {
    state.system.substate =
        this->recirculation_->state ? tion4s_state_t::SUBSTATE_RECIRCULATION : tion4s_state_t::SUBSTATE_INFLOW;
  }

  state.system.target_temperature = this->target_temperature;

  if (this->led_) {
    state.system.led_state = this->led_->state;
  }

  if (this->buzzer_) {
    state.system.sound_state = this->buzzer_->state;
  }

  if (this->custom_fan_mode.has_value()) {
    state.system.fan_speed = this->get_fan_speed();
  }
}

/*
void Tion4s::frameRecieveCallback(frame_type_t type, const void *data, uint16_t data_size) {
  ESP_LOGD(TAG, "frameRecieveCallback 0x%04X: %s", type,
           format_hex_pretty(static_cast<const uint8_t *>(data), data_size).c_str());

  switch (type) {

    case FRAME_TYPE_GET_ERR_CNT_RESP: {
      constexpr auto ERROR_TYPE_NUMBER = 32;
      typedef struct {
        uint8_t er[ERROR_TYPE_NUMBER];
      } PACKED BRLT_ErrorCnt_TypeDef;
      typedef struct {
        int32_t CmdID;
        BRLT_ErrorCnt_TypeDef cnt;
      } PACKED brlt_error_data;
      if (data_size != sizeof(brlt_error_data)) {
        ESP_LOGW(TAG, "Incorrect response data size: %u", data_size);
      }
      auto &err_data = *static_cast<const brlt_error_data *>(data);
      for (size_t i = 0; i < ERROR_TYPE_NUMBER; i++) {
        ESP_LOGI(TAG, "err_cnt[%u]: %u", i, err_data.cnt.er[i]);
      }
      break;
    }
    case FRAME_TYPE_GET_CURR_TEST_REQ: {
      if (data_size != sizeof(uint32_t)) {
        ESP_LOGW(TAG, "Incorrect response data size: %u", data_size);
      }
      auto &test_type = *static_cast<const uint32_t *>(data);
      ESP_LOGI(TAG, "test_type: %u", test_type);
      break;
    }
    case FRAME_TYPE_GET_TIMER_RESP: {
      typedef struct {
        uint8_t Monday : 1;
        uint8_t Tuesday : 1;
        uint8_t Wednesday : 1;
        uint8_t Thursday : 1;
        uint8_t Friday : 1;
        uint8_t Saturday : 1;
        uint8_t Sunday : 1;
        uint8_t reserved : 1;
        uint8_t hours;
        uint8_t minutes;
      } PACKED br4s_timer_time_t;
      typedef struct {
        uint8_t power_state : 1;
        uint8_t sound_ind_state : 1;
        uint8_t led_ind_state : 1;
        uint8_t heater_mode : 1;  // 0 - temperature maintenance, 1 - heating //!!!отличается от HeasterMode!!!
        uint8_t timer_state : 1; // on/off
        uint8_t reserved : 3;
      } PACKED br4s_timer_flags_t;
      typedef struct {
        br4s_timer_time_t timer_time;
        br4s_timer_flags_t timer_flags;
        int8_t target_temp;
        uint8_t fan_state;
        uint8_t device_mode;
      } PACKED br4s_timer_settings_t;
      typedef struct _tagBR4S_RC_RF_FrameTimerSettingsResponse_TypeDef {
        uint32_t cmd_id;
        uint8_t timer_id;
        br4s_timer_settings_t timer;
      } PACKED BRLT_RC_RF_TimerSettings_Resp_TypeDef;
      if (data_size != sizeof(BRLT_RC_RF_TimerSettings_Resp_TypeDef)) {
        ESP_LOGW(TAG, "Incorrect response data size: %u", data_size);
      }
      auto &b_timer_set = *static_cast<const BRLT_RC_RF_TimerSettings_Resp_TypeDef *>(data);
      ESP_LOGI(TAG, "timer[%u].device_mode: %u", b_timer_set.timer_id, b_timer_set.timer.device_mode);
      ESP_LOGI(TAG, "timer[%u].target_temp: %d", b_timer_set.timer_id, b_timer_set.timer.target_temp);
      ESP_LOGI(TAG, "timer[%u].fan_state: %u", b_timer_set.timer_id, b_timer_set.timer.fan_state);
      // timer[b_timer_set.timer_id] = b_timer_set.timer;
      break;
    }
    case FRAME_TYPE_GET_TIMERS_STATE_RESP: {
      ESP_LOGW(TAG, "FRAME_TYPE_GET_TIMERS_STATE_RESP response: %s",
               format_hex_pretty(static_cast<const uint8_t *>(data), data_size).c_str());
      break;
    }
    case FRAME_TYPE_GET_TIME_RESP: {
      typedef struct {
        int64_t unix_time;
      } PACKED br4s_time_format_t;
      typedef struct _tagBR4S_RC_time_Resp_TypeDef {
        uint32_t cmd_id;
        br4s_time_format_t time;
      } PACKED br4s_rc_time_pack_t;
      if (data_size != sizeof(br4s_rc_time_pack_t)) {
        ESP_LOGW(TAG, "Incorrect response data size: %u", data_size);
      }
      auto &b_time_pack = *static_cast<const br4s_rc_time_pack_t *>(data);
      ESP_LOGI(TAG, "time.unix_time: %" PRId64, b_time_pack.time.unix_time);
      // currDateTime = QDateTime::fromSecsSinceEpoch(b_time_pack.time.unix_time, Qt::UTC);
      break;
    }
    case FRAME_TYPE_GET_TURBO_PARAM_RESP: {
      typedef struct _tagbr4s_turbo_state_t {
        uint8_t is_active;
        uint16_t turbo_time;
        uint8_t err_code;
      } PACKED br4s_turbo_state_t;
      typedef struct _tagbr4s_turbo_state_pack_t {
        uint32_t cmd_id;
        br4s_turbo_state_t turbo_state;
      } PACKED br4s_rc_turbo_state_pack_t;
      if (data_size != sizeof(br4s_rc_turbo_state_pack_t)) {
        ESP_LOGW(TAG, "Incorrect response data size: %u", data_size);
      }
      auto &b_turbo_pack = *static_cast<const br4s_rc_turbo_state_pack_t *>(data);
      ESP_LOGI(TAG, "turbo_state.is_active : %u", b_turbo_pack.turbo_state.is_active);
      ESP_LOGI(TAG, "turbo_state.turbo_time: %u", b_turbo_pack.turbo_state.turbo_time);
      ESP_LOGI(TAG, "turbo_state.err_code  : %u", b_turbo_pack.turbo_state.err_code);
      // turbo_state = b_turbo_pack.turbo_state;
      break;
    }
    default:
      ESP_LOGW(TAG, "Unsupported frame type 0x%04X: %s", type,
               format_hex_pretty(static_cast<const uint8_t *>(data), data_size).c_str());
      break;
  }
}
*/
// br4s/br4s_protocol.cpp
// setSubstate
// setTemp
// setSpeed
// setPowerState
// setTestState
// resetFilter
// resetHard
// switchSound
// switchLED
// switchMA - magic air
// switchMA_auto
// setHeaterMode
// setNoTest
// BR4S_Rc::RawDataList BR4S_Rc::setGateTest(void)
// BR4S_Rc::RawDataList BR4S_Rc::setHeaterTest(void)
// BR4S_Rc::RawDataList BR4S_Rc::setDefaultTest(void)
// BR4S_Rc::RawDataList BR4S_Rc::setTime (int64_t unix_time, int64_t timezone_offset)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerEnable(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerTime(QTime time, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerMonday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerTuesday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerWednesday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerThursday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerFriday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerSaturday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerSunday(bool state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerSubstate(uint8_t substate, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerHeaterMode(bool heater_mode, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerPowerState(bool power_state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerSndIndState(bool snd_ind_state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerLEDIndState(bool led_ind_state, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerTargetSpeed(uint8_t speed, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::setTimerTargetTemp(int8_t temp, uint8_t timer_id)
// BR4S_Rc::RawDataList BR4S_Rc::StartTurbo(int16_t time)
// BR4S_Rc::RawDataList BR4S_Rc::ResetTurboErr()
// BR4S_Rc::RawDataList BR4S_Rc::TurboReq()

// void Tion4s::paramRequest() {
//   ESP_LOGD(TAG, "paramRequest");
//   this->sendFrame(FRAME_TYPE_GET_MODE_REQ);
// }

// void Tion4s::devRequest() {
//   ESP_LOGD(TAG, "devRequest");
//   this->sendFrame(FRAME_TYPE_DEV_STATUS_REQ);
// }

// void Tion4s::ErrCntRequest() {
//   ESP_LOGD(TAG, "ErrCntRequest");
//   this->sendFrame(FRAME_TYPE_GET_ERR_CNT_REQ);
// }

// void Tion4s::CurrTestRequest() {
//   ESP_LOGD(TAG, "CurrTestRequest");
//   this->sendFrame(FRAME_TYPE_GET_CURR_TEST_REQ);
// }

// void Tion4s::timeRequest() {
//   ESP_LOGD(TAG, "timeRequest");
//   this->sendFrame(FRAME_TYPE_GET_TIME_REQ);
// }

// void Tion4s::TurboReq() {
//   ESP_LOGD(TAG, "TurboReq");
//   this->sendFrame(FRAME_TYPE_GET_TURBO_PARAM_REQ);
// }

// void Tion4s::timerSettingsRequest(uint8_t timer_id) {
//   ESP_LOGD(TAG, "timerSettingsRequest %u", timer_id);
//   struct {
//     uint32_t cmd_id;
//     uint8_t timer_id;
//   } PACKED timer_req{.cmd_id = 1, .timer_id = timer_id};
//   this->sendFrame(FRAME_TYPE_GET_TIMER_REQ, &timer_req, sizeof(timer_req));
// }

}  // namespace tion

}  // namespace esphome
