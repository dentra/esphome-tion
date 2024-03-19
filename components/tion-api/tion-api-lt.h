#pragma once

#include <functional>

#include "tion-api.h"
#include "tion-api-lt-internal.h"
#include "tion-api-defines.h"

namespace dentra {
namespace tion {

class TionLtApi : public TionApiBase {
 public:
  TionLtApi();

  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool write_state(const TionState &state, uint32_t request_id) const;
  bool reset_filter(const TionState &state, uint32_t request_id = 1) const;
  bool factory_reset(const TionState &state, uint32_t request_id = 1) const;
  bool reset_errors(const TionState &state, uint32_t request_id = 1) const;

  void set_button_presets(const dentra::tion_lt::button_presets_t &button_presets);

  void request_state() override;
  void write_state(TionStateCall *call) override {
    this->write_state(this->make_write_state_(call), ++this->request_id_);
  }
  void reset_filter() override { this->reset_filter(this->state_, ++this->request_id_); }

 protected:
  tion_lt::button_presets_t button_presets_{
      .tmp{
          TION_LT_BUTTON_PRESET_TMP1,
          TION_LT_BUTTON_PRESET_TMP2,
          TION_LT_BUTTON_PRESET_TMP3,
      },
      .fan{
          TION_LT_BUTTON_PRESET_FAN1,
          TION_LT_BUTTON_PRESET_FAN2,
          TION_LT_BUTTON_PRESET_FAN3,
      },
  };

  bool request_dev_info_() const;
  bool request_state_() const;

  void dump_state_(const tion_lt::tionlt_state_t &state) const;
  void update_state_(const tion_lt::tionlt_state_t &state);
  void update_dev_info_(const tion::tion_dev_info_t &dev_info);

  uint8_t calc_productivity(const tion_lt::tionlt_state_t &state) const {
    const auto prev_fan_time = this->state_.fan_time;
    if (prev_fan_time == 0) {
      return 0;
    }
    const auto diff_time = state.counters.fan_time - prev_fan_time;
    if (diff_time == 0) {
      return 0;
    }
    const auto prev_airflow_counter = this->state_.airflow_counter;
    const auto diff_airflow = state.counters.airflow_counter - prev_airflow_counter;
    // return (float(diff_airflow) / float(diff_time) * state.counters.airflow_k());
    return state.counters.airflow_mult(float(diff_airflow) / float(diff_time));
  }
};

}  // namespace tion
}  // namespace dentra
