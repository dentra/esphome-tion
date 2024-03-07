#pragma once

#include "../tion-api/tion-api-4s.h"
#include "../tion/tion_controls.h"
#include "../tion/tion_api_component.h"

namespace esphome {
namespace tion {

using namespace dentra::tion;

using Tion4sApi = dentra::tion_4s::Tion4sApi;

template<class parent_t> class TionRecirculationSwitch : public Parented<parent_t>, public switch_::Switch {
 public:
  explicit TionRecirculationSwitch(parent_t *parent) : Parented<parent_t>(parent) {}
  void write_state(bool state) override { this->parent_->control_recirculation_state(state); }
};

class Tion4sApiComponent : public TionApiComponentBase<Tion4sApi> {
 public:
  explicit Tion4sApiComponent(Tion4sApi *api, TionVPortType vport_type) : TionApiComponentBase(api, vport_type) {
    if (this->vport_type_ == TionVPortType::VPORT_BLE) {
      api->enable_native_boost_support();
    }
  }

  void setup() override;
  void dump_config() override;

  void set_heartbeat_interval(uint32_t heartbeat_interval) {
#ifdef TION_ENABLE_HEARTBEAT
    this->heartbeat_interval_ = heartbeat_interval;
#endif
  }

 protected:
#ifdef TION_ENABLE_HEARTBEAT
  uint32_t heartbeat_interval_{5000};
#endif
};

}  // namespace tion
}  // namespace esphome
