#pragma once

#include "esphome/core/preferences.h"

#include "../tion-api/tion-api-3s.h"
#include "../tion-api/tion-api-ble-3s.h"
#include "../tion/tion_vport_ble.h"

namespace esphome {
namespace tion {

class Tion3sBleVPort;

class Tion3sBleIO : public TionBleIO<dentra::tion::Tion3sBleProtocol> {
 public:
  explicit Tion3sBleIO() { this->set_ble_encryption(esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT); }

  bool ble_reg_for_notify() const override;

  void set_vport(Tion3sBleVPort *vport) { this->vport_ = vport; }

 protected:
  Tion3sBleVPort *vport_{};
};

class Tion3sBleVPort : public TionVPortBLEComponent<Tion3sBleIO> {
 public:
  Tion3sBleVPort(io_type *io) : TionVPortBLEComponent(io) {
    this->io_->set_on_ready(io_type::on_ready_type::create<Tion3sBleVPort, &Tion3sBleVPort::on_ready_3s_>(*this));
  }

  void setup() override;
  void dump_config() override;

  void write(const io_type::frame_spec_type &frame, size_t size) override;

  void set_experimental_always_pair(bool value) { this->experimental_always_pair_ = value; }

  void pair();
  void reset_pair();
  bool is_paired() const { return this->pair_state_ > 0; }

  void set_api(dentra::tion::Tion3sApi *api) { this->api_ = api; }

 protected:
  ESPPreferenceObject rtc_;

  int8_t pair_state_{};  // 0: not paired, >0: paired, <0: pairing
  bool experimental_always_pair_{};
  dentra::tion::Tion3sApi *api_{};

  void on_ready_3s_();
  void pair_();
};

}  // namespace tion
}  // namespace esphome
