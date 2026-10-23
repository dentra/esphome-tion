#pragma once

#include "esphome/core/defines.h"

#ifdef USE_VPORT_BLE

#include <functional>

#include "esphome/components/vport/vport_ble.h"

#include "tion_vport.h"

namespace esphome {
namespace tion {

#define TION_VPORT_BLE_LOG(port_name) VPORT_BLE_LOG(port_name);

template<class protocol_type> class TionBleIO : public TionIO<protocol_type>, public vport::VPortBLENode {
 public:
  using frame_spec_type = typename protocol_type::frame_spec_type;

  using on_ready_type = std::function<void()>;

  explicit TionBleIO() {
    this->protocol_.set_protocol_writer([this](const uint8_t *data, size_t size) { return this->write_(data, size); });
    this->set_ble_service(this->protocol_.get_ble_service());
    this->set_ble_char_tx(this->protocol_.get_ble_char_tx());
    this->set_ble_char_rx(this->protocol_.get_ble_char_rx());
    this->set_ble_disconnect_on_auth_fail(true);
  }

  void set_on_ready(on_ready_type &&on_ready) { this->on_ready_ = std::move(on_ready); }

  void on_ble_ready() override {
    if (this->on_ready_) {
      this->on_ready_();
    }
  }
  bool on_ble_data(const uint8_t *data, uint16_t size) override { return this->protocol_.read_data(data, size); }

 protected:
  on_ready_type on_ready_;

  bool write_(const uint8_t *data, size_t size) { return this->write_ble_data(data, size); }
};

template<class io_t>
class TionVPortBLEComponent : public vport::VPortBLEComponent<io_t, typename io_t::frame_spec_type> {
 public:
  TionVPortBLEComponent(io_t *io) : vport::VPortBLEComponent<io_t, typename io_t::frame_spec_type>(io) {}

  TionVPortType get_type() const { return TionVPortType::VPORT_BLE; }
};

}  // namespace tion
}  // namespace esphome

#endif  // USE_VPORT_BLE
