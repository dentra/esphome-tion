#pragma once

#include "../vport_ble/vport_ble.h"
#include "../tion/tion.h"
#include "../tion-api/tion-api-ble-lt.h"

namespace esphome {
namespace tion {

class TionBLEVPortSettings {
 public:
  virtual const char *get_ble_service() const = 0;
  virtual const char *get_ble_char_tx() const = 0;
  virtual const char *get_ble_char_rx() const = 0;
  virtual esp_ble_sec_act_t get_ble_encryption() { return static_cast<esp_ble_sec_act_t>(0); }
};

class TionBLEVPort : public vport::VPortBLEComponent<uint16_t>, public TionVPort {
 public:
  void dump_component_config(const char *TAG) const;
  void dump_config() override;
  void update() override;

  // VPortBLEComponent implementation
  void on_ble_ready() override;
  void read_ble_data(const uint8_t *data, uint16_t size) override { this->protocol_->read_data(data, size); }

  void schedule_disconnect(uint32_t timeout = 3000);
  void cancel_disconnect();
  void frame_state_disconnect(uint16_t frame_type);

  void set_state_type(uint16_t state_type) { this->state_type_ = state_type; }
  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; }
  void set_persistent_connection(bool persistent_connection) { this->persistent_connection_ = persistent_connection; }
  bool is_persistent_connection() const { return this->persistent_connection_; }

  void configure(TionBLEVPortSettings *settings) {
    this->set_ble_service(settings->get_ble_service());
    this->set_ble_char_tx(settings->get_ble_char_tx());
    this->set_ble_char_rx(settings->get_ble_char_rx());
    if (settings->get_ble_encryption() != 0) {
      this->set_ble_encryption(settings->get_ble_encryption());
    }
  }

  // TionVport implementation
  void set_protocol(dentra::tion::TionBleProtocol *protocol) { this->protocol_ = protocol; }
  dentra::tion::TionProtocol *get_protocol() const override { return this->protocol_; }
  TionVPort::Type get_vport_type() const override { return TionVPort::Type::VPORT_UART; }

  void schedule_poll() override final;

 protected:
  uint32_t state_timeout_{};
  bool persistent_connection_{};
  uint16_t state_type_{};
  dentra::tion::TionBleProtocol *protocol_{};
};

template<class T> class VPortTionBleProtocol : public T, public TionBLEVPortSettings, public Parented<TionBLEVPort> {
 public:
  VPortTionBleProtocol(TionBLEVPort *vport) : Parented(vport) {
    vport->set_protocol(this);
    vport->configure(this);
  }
  bool read_frame(uint16_t type, const void *data, size_t size) override {
    if (this->parent_ == nullptr) {
      return false;
    }
    this->parent_->fire_listeners(type, data, size);
    this->parent_->frame_state_disconnect(type, data, size);
    return true;
  }
  bool write_data(const uint8_t *data, size_t size) const override { return parent_->write_ble_data(data, size); }

  const char *get_ble_service() const override { return T::get_ble_service(); }
  const char *get_ble_char_tx() const override { return T::get_ble_char_tx(); }
  const char *get_ble_char_rx() const override { return T::get_ble_char_rx(); }
};

class VPortTionBleLtProtocol final : public VPortTionBleProtocol<dentra::tion::TionBleLtProtocol> {
 public:
  VPortTionBleLtProtocol(TionBLEVPort *vport) : VPortTionBleProtocol(vport) {}
};

}  // namespace tion
}  // namespace esphome
