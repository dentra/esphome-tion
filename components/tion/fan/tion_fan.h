#pragma once
#include "esphome/core/defines.h"
#ifdef USE_FAN

#include <map>

#include "esphome/components/fan/fan.h"

#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_vport.h"
#include "tion_helpers.h"

namespace esphome {
namespace tion {

class TionFanComponentBase : public fan::Fan, public TionComponent {
 public:
  fan::FanTraits get_traits() override;

  virtual void control_fan_state(bool state, uint8_t speed) = 0;

 protected:
  void control(const fan::FanCall &call) override;
#ifdef TION_ENABLE_PRESETS
  using TionFanPresetData = TionPresetData<bool, false>;
  std::map<std::string, TionFanPresetData> presets_;
#endif
};

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type> class TionFanComponent : public TionFanComponentBase {
  using tion_state_type = typename tion_api_type::state_type;

 public:
  explicit TionFanComponent(tion_api_type *api, TionVPortType vport_type) : api_(api), vport_type_(vport_type) {
    // using this_t = std::remove_pointer_t<decltype(this)>;
    // this->api_->on_dev_info.template set<this_t, &this_t::on_dev_info>(*this);
    // this->api_->on_state.template set<this_t, &this_t::on_state>(*this);
    // this->api_->set_on_ready(tion_api_type::on_ready_type::template create<this_t, &this_t::on_ready>(*this));
  }

 protected:
  tion_api_type *api_;
  const TionVPortType vport_type_;
};

}  // namespace tion
}  // namespace esphome
#endif  // USE_FAN
