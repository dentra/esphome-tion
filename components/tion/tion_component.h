#pragma once

#include <functional>

#include "esphome/core/defines.h"
#include "esphome/core/version.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include "../tion-api/tion-api.h"
#ifdef USE_TION_O2
#include "../tion-api/tion-api-o2.h"
#endif
#ifdef USE_TION_3S
#include "../tion-api/tion-api-3s.h"
#endif
#ifdef USE_TION_4S
#include "../tion-api/tion-api-4s.h"
#endif
#ifdef USE_TION_LT
#include "../tion-api/tion-api-lt.h"
#endif
#include "tion_vport.h"
#include "tion_prefs.h"

#define TION_C_LOG_(log, tag, fmt, ...) log(tag, "'%s' - " fmt, this->get_name().c_str(), ##__VA_ARGS__)
#define TION_C_LOGV(tag, fmt, ...) TION_C_LOG_(ESP_LOGV, tag, fmt, ##__VA_ARGS__)
#define TION_C_LOGD(tag, fmt, ...) TION_C_LOG_(ESP_LOGD, tag, fmt, ##__VA_ARGS__)
#define TION_C_LOGI(tag, fmt, ...) TION_C_LOG_(ESP_LOGI, tag, fmt, ##__VA_ARGS__)
#define TION_C_LOGW(tag, fmt, ...) TION_C_LOG_(ESP_LOGW, tag, fmt, ##__VA_ARGS__)

namespace esphome {
namespace tion {

class TionApiComponent : public PollingComponent {
 protected:
  using TionApiBase = dentra::tion::TionApiBase;
  using TionState = dentra::tion::TionState;
  using TionStateCall = dentra::tion::TionStateCall;
  using TionGatePosition = dentra::tion::TionGatePosition;

  class BatchStateCall : public dentra::tion::TionStateCall {
   public:
    explicit BatchStateCall(TionApiComponent *c) : dentra::tion::TionStateCall(c->api_), c_(c) {}

    virtual ~BatchStateCall() {}

    void perform() override;

    uint32_t get_start_time() const { return this->start_time_; };

   protected:
    TionApiComponent *c_;
    uint32_t start_time_{};
    void perform_();
  };

 public:
  explicit TionApiComponent(TionApiBase *api) : api_(api), batch_call_(this) {
    api->set_on_state(
        [this](const TionState &state, const uint32_t request_id) { this->on_state_(state, request_id); });
  }

  void dump_config() override;
  void call_setup() override;
  // обработка и обновление App.app_state_ происходит только для компонентов
  // переопределяющих looping_components (см. application.cpp)
  void loop() override {}

  float get_setup_priority() const override { return setup_priority::AFTER_CONNECTION; }

  void update() override;

  /**
   * Add a callback for the breezer state, each time the state of the device is updated, this callback will be called.
   * When state is not returned in configured period - a callback called with nullptr.
   *
   * @param callback The callback to call.
   */
  void add_on_state_callback(std::function<void(const TionState *)> &&callback) {
    this->state_callback_.add(std::move(callback));
  }

#ifdef TION_ENABLE_API_CONTROL_CALLBACK
  /**
   * Add a callback for the breezer configuration, each time the configuration parameters of a device
   * is updated (using perform() of a TionStateCall), this callback will be called, before any on_state callback.
   *
   * @param callback The callback to call.
   */
  void add_on_control_callback(std::function<void(TionStateCall *)> &&callback) {
    this->control_callback_.add(std::move(callback));
  }
#endif

  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; };
  void set_batch_timeout(uint32_t batch_timeout) { this->batch_timeout_ = batch_timeout; };
  void set_force_update(bool force_update) { this->force_update_ = force_update; };
  bool get_force_update() const { return this->force_update_; }
  void add_preset(const char *name, const TionApiBase::PresetData &preset) { this->api_->add_preset(name, preset); }

  TionStateCall *make_call();

  TionApiBase *api() { return this->api_; }

  bool has_state() const { return !this->status_has_error(); }

  const dentra::tion::TionTraits &traits() const { return this->api_->get_traits(); }
  const dentra::tion::TionState &state() const { return this->api_->get_state(); }

  void auto_update(float co2_value) {
    if (std::isnan(co2_value)) {
      return;
    }
    auto current_co2 = static_cast<uint16_t>(co2_value);
    if (current_co2 < TION_AUTO_MIN_SETPOINT) {
      return;
    }
    auto *call = this->make_call();
    if (this->api_->auto_update(current_co2, call)) {
      call->perform();
    }
  }

  void boost_enable(bool state) {
    // для режима турбо не применяем BatchCall чтобы ни в коем случае не смешать вызовы.
    TionStateCall call(this->api_);
    this->api_->enable_boost(state, &call);
    call.perform();
  }

#ifdef USE_TION_RESTORE_STATE
  void set_rtc_key(const char *key) { this->rtc_hash_ = key ? fnv1_hash(key) : 0; }
#endif

 protected:
  TionApiBase *api_;
  bool force_update_{};

  BatchStateCall batch_call_;

  uint32_t state_timeout_{};
  uint32_t batch_timeout_{};

#ifdef USE_TION_RESTORE_STATE
  uint32_t rtc_hash_{};
  TionPreferenceObject rtc_pref_;
#endif
  bool load_state_();
  void save_state_();

  CallbackManager<void(const TionState *)> state_callback_{};
#ifdef TION_ENABLE_API_CONTROL_CALLBACK
  CallbackManager<void(TionStateCall *)> control_callback_{};
#endif

  void on_state_(const TionState &state, const uint32_t request_id);
  void state_check_schedule_();
};

// T - TionApi implementation
template<class A> class TionApiComponentBase : public TionApiComponent {
 public:
  using Api = A;

  explicit TionApiComponentBase(Api *api, TionVPortType /*vport_type*/) : TionApiComponent(api) {}

 protected:
  Api *typed_api() { return reinterpret_cast<Api *>(this->api_); }
};

#ifdef USE_TION_O2
class TionO2ApiComponent : public TionApiComponentBase<dentra::tion_o2::TionO2Api> {
 public:
  explicit TionO2ApiComponent(TionApiComponentBase::Api *api, TionVPortType vport_type)
      : TionApiComponentBase(api, vport_type) {}
  void setup() override {
    this->set_timeout(200, [api = this->typed_api()]() { api->update_work_mode(); });
  }
};
#endif

#ifdef USE_TION_3S
using Tion3sApiComponent = TionApiComponentBase<dentra::tion::Tion3sApi>;
#endif

#ifdef USE_TION_4S
class Tion4sApiComponent : public TionApiComponentBase<dentra::tion_4s::Tion4sApi> {
 public:
  explicit Tion4sApiComponent(TionApiComponentBase::Api *api, TionVPortType vport_type)
      : TionApiComponentBase(api, vport_type) {
    if (vport_type == TionVPortType::VPORT_BLE) {
      api->enable_native_boost_support();
    }
#ifdef TION_ENABLE_SCHEDULER
    this->typed_api()->set_on_time([this](time_t time, uint32_t request_id) { this->on_time(time, request_id); });
    this->typed_api()->set_on_timer([this](uint8_t timer_id, const dentra::tion_4s::tion4s_timer_t &timer,
                                           uint32_t request_id) { this->on_timer(timer_id, timer, request_id); });
    this->typed_api()->set_on_timers_state(
        [this](const dentra::tion_4s::tion4s_timers_state_t &timers_state, uint32_t request_id) {
          this->on_timers_state(timers_state, request_id);
        });
#endif
  }
#ifdef TION_ENABLE_SCHEDULER
  void on_time(time_t time, uint32_t request_id);
  void on_timer(uint8_t timer_id, const dentra::tion_4s::tion4s_timer_t &timer, uint32_t request_id);
  void on_timers_state(const dentra::tion_4s::tion4s_timers_state_t &timers_state, uint32_t request_id);
  void dump_timers();
  void reset_timers();
#endif
};
#endif

#ifdef USE_TION_LT
class TionLtApiComponent : public TionApiComponentBase<dentra::tion::TionLtApi> {
 public:
  explicit TionLtApiComponent(TionApiComponentBase::Api *api, TionVPortType vport_type)
      : TionApiComponentBase(api, vport_type) {
    if (vport_type == TionVPortType::VPORT_UART) {
      api->enable_kiv_support();
    }
  }

  void set_button_presets(const dentra::tion_lt::button_presets_t &button_presets) {
    this->typed_api()->set_button_presets(button_presets);
  }
};
#endif

}  // namespace tion
}  // namespace esphome
