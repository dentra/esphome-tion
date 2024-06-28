#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"

#include "esphome/components/esp32_ble_server/ble_characteristic.h"
#include "esphome/components/esp32_ble_server/ble_server.h"
#include "esphome/components/switch/switch.h"

#include "../tion/tion_component.h"

#ifndef TION_RC_DUMP
#define TION_RC_DUMP ESP_LOGV
#endif

namespace esphome {
namespace tion_rc {

using namespace esp32_ble_server;

template<class P> class TionRCControlProtocol {
 public:
  TionRCControlProtocol() {
    using this_t = std::remove_pointer_t<decltype(this)>;
    this->pr_.reader.template set<this_t, &this_t::pr_on_frame_>(*this);
    this->pr_.writer.template set<this_t, &this_t::pr_do_write_>(*this);
  }

  virtual void on_frame(uint16_t type, const uint8_t *data, size_t size) = 0;

 protected:
  void pr_on_frame_(const typename P::frame_spec_type &data, size_t size) {
    this->on_frame(data.type, data.data, size - data.head_size());
  }

  // FIXME directly assign to this->pr_.writer
  bool pr_do_write_(const uint8_t *data, size_t size) {
    if (this->pr_writer_) {
      this->pr_writer_(data, size);
    }
    return true;
  }

  P pr_;
  std::function<void(const uint8_t *data, size_t size)> pr_writer_;
};

class TionRCControl {
 public:
  TionRCControl(dentra::tion::TionApiBase *api) : api_(api) {}
  virtual void adv(bool pair) = 0;
  virtual void on_state(const dentra::tion::TionState &st) = 0;

  virtual void pr_read_data(const uint8_t *data, size_t size) = 0;

  virtual void set_writer(std::function<void(const uint8_t *data, size_t size)> &&writer) = 0;

  bool has_state_req() const { return this->state_req_id_ != 0; }

  virtual const char *get_ble_service() const = 0;
  virtual const char *get_ble_char_rx() const = 0;
  virtual const char *get_ble_char_tx() const = 0;

  esp32_ble::ESPBTUUID get_ble_service_uuid() const { return esp32_ble::ESPBTUUID::from_raw(this->get_ble_service()); }

 protected:
  dentra::tion::TionApiBase *api_;
  uint32_t state_req_id_{};
};

template<class P> class TionRCControlImpl : public TionRCControl, public TionRCControlProtocol<P> {
 public:
  TionRCControlImpl(dentra::tion::TionApiBase *api) : TionRCControl(api) {}

  void pr_read_data(const uint8_t *data, size_t size) override { this->pr_.read_data(data, size); }

  void set_writer(std::function<void(const uint8_t *data, size_t size)> &&writer) override {
    this->pr_writer_ = std::move(writer);
  }

  const char *get_ble_service() const override { return this->pr_.get_ble_service(); }
  const char *get_ble_char_rx() const override { return this->pr_.get_ble_char_rx(); }
  const char *get_ble_char_tx() const override { return this->pr_.get_ble_char_tx(); }
};

class TionRC final : public Component, public BLEServiceComponent, public GATTsEventHandler, public GAPEventHandler {
 public:
  TionRC(tion::TionApiComponent *tion, TionRCControl *control);
  float get_setup_priority() const override { return setup_priority::AFTER_BLUETOOTH; }
  void loop() override;

  void on_client_connect() override;
  void on_client_disconnect() override;
  void start() override {}
  void stop() override;

  void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                           esp_ble_gatts_cb_param_t *param) override;
  void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) override;

  void set_pair_mode(switch_::Switch *pair_mode) { this->pair_mode_ = pair_mode; }

  void adv(bool pair);

 protected:
  TionRCControl *control_;
  BLEService *service_{};
  BLECharacteristic *char_notify_{};
  switch_::Switch *pair_mode_{};
  void setup_service_();
  enum class State { STARTED, STOPPED, INITIALIZED, STARTING } state_{State::STOPPED};
};

class TionRCPairSwitch : public Component, public switch_::Switch, public Parented<TionRC> {
 public:
 protected:
  void write_state(bool state) override {
    this->parent_->adv(state);
    this->publish_state(state);
  }
};

}  // namespace tion_rc
}  // namespace esphome
