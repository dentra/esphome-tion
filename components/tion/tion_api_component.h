#pragma once

#include <functional>
#include <map>

#include "esphome/core/defines.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

#include "../tion-api/tion-api.h"
#include "tion_vport.h"
#include "tion_batch_call.h"

namespace esphome {
namespace tion {

class TionApiComponent : public PollingComponent {
 protected:
  using TionApiBase = dentra::tion::TionApiBase;
  using TionState = dentra::tion::TionState;
  using TionStateCall = dentra::tion::TionStateCall;
  using TionGatePosition = dentra::tion::TionGatePosition;

  constexpr static const auto *TAG = "tion_api_component";

 public:
  explicit TionApiComponent(TionApiBase *api, TionVPortType vport_type) : api_(api), vport_type_(vport_type) {
    api->on_state_fn.set<TionApiComponent, &TionApiComponent::on_state_>(*this);
  }

  void dump_config() override;
  void call_setup() override;
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

  /**
   * Add a callback for the breezer configuration, each time the configuration parameters of a device
   * is updated (using perform() of a StateCall), this callback will be called, before any on_state callback.
   *
   * @param callback The callback to call.
   */
  // void add_on_control_callback(std::function<void(StateCall &)> &&callback);

  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; };
  void set_batch_timeout(uint32_t batch_timeout) { this->batch_timeout_ = batch_timeout; };
  void set_force_update(bool force_update) { this->force_update_ = force_update; };
  bool get_force_update() const { return this->force_update_; }
  void add_preset(const std::string &name, const TionApiBase::PresetData &preset) {
    this->api_->add_preset(name, preset);
  }

  TionStateCall *make_call();

  TionApiBase *api() { return this->api_; }

  bool has_state() const { return this->has_state_; }

  const dentra::tion::TionTraits &traits() const { return this->api_->traits(); }
  const dentra::tion::TionState &state() const { return this->api_->get_state(); }

  void boost_enable();
  void boost_cancel();

 protected:
  TionApiBase *api_;
  TionVPortType vport_type_;
  bool has_state_{};
  bool force_update_{};
  BatchStateCall *batch_call_{};

  uint32_t state_timeout_{};
  uint32_t batch_timeout_{};

  CallbackManager<void(const TionState *)> state_callback_{};
  // CallbackManager<void(StateCall &)> control_callback_{};

  void on_state_(const TionState &state, const uint32_t request_id);
  void state_check_schedule_();
  void state_check_cancel_();
  void state_check_report_(uint32_t timeout);
  void batch_write_();
};

// T - TionApi implementation
template<class A> class TionApiComponentBase : public TionApiComponent {
  // static_assert(std::is_base_of_v<dentra::tion::TionApi, A>,
  //               "A is not derived from dentra::tion::TionApi");

 public:
  explicit TionApiComponentBase(A *api, TionVPortType vport_type) : TionApiComponent(api, vport_type) {}

 protected:
  A *typed_api() { return reinterpret_cast<A *>(this->api_); }
};

}  // namespace tion
}  // namespace esphome
