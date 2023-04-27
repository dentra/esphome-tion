#pragma once
#ifdef USE_VPORT_BLE

#include "esphome/components/vport/vport_ble.h"
#include "tion_vport.h"

namespace esphome {
namespace tion {

#define TION_VPORT_BLE_LOG(port_name) \
  VPORT_BLE_LOG(port_name); \
  ESP_LOGCONFIG(TAG, "  State timeout: %s", \
                this->state_timeout_ > 0 ? (to_string(this->state_timeout_ / 1000) + "s").c_str() : ONOFF(false));

template<class protocol_type> class TionBleIO : public TionIO<protocol_type>, public vport::VPortBLENode {
 public:
  using frame_spec_type = typename protocol_type::frame_spec_type;

  using on_ready_type = etl::delegate<void()>;

  explicit TionBleIO() {
    using this_t = typename std::remove_pointer<decltype(this)>::type;
    this->protocol_.writer.template set<this_t, &this_t::write_>(*this);
    this->set_ble_service(this->protocol_.get_ble_service());
    this->set_ble_char_tx(this->protocol_.get_ble_char_tx());
    this->set_ble_char_rx(this->protocol_.get_ble_char_rx());
  }

  void set_on_ready(on_ready_type &&on_ready) { this->on_ready_ = std::move(on_ready); }

  void on_ble_ready() override { this->on_ready_.call_if(); }
  bool on_ble_data(const uint8_t *data, uint16_t size) override { return this->protocol_.read_data(data, size); }

 protected:
  on_ready_type on_ready_;

  bool write_(const uint8_t *data, size_t size) { return this->write_ble_data(data, size); }
};

template<class io_t, class frame_spec_t>
class TionVPortBLEComponent : public vport::VPortBLEComponent<io_t, frame_spec_t> {
 public:
  TionVPortBLEComponent(io_t *io) : vport::VPortBLEComponent<io_t, frame_spec_t>(io) {}

  bool can_disconnect(const frame_spec_t &frame, size_t size) {
    return this->state_type_ == 0 || this->state_type_ == frame.template type;
  }

  TionVPortType get_vport_type() const { return TionVPortType::VPORT_BLE; }

  void set_state_type(uint16_t state_type) { this->state_type_ = state_type; }
  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; }

 protected:
  uint32_t state_timeout_{};  // FIXME разобраться с применением
  uint16_t state_type_{};
};

}}

#endif  // USE_VPORT_BLE
