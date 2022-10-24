#pragma once
#ifdef USE_VPORT_BLE
#include "esphome/components/ble_client/ble_client.h"
#include "vport_component.h"

// #define VPORT_BLE_ENABLE_QUEUE
#ifdef VPORT_BLE_ENABLE_QUEUE
#include "etl/circular_buffer.h"
#endif

namespace esphome {
namespace vport {

#define VPORT_BLE_LOG(tag, port_name) \
  VPORT_LOG(tag, port_name); \
  ESP_LOGCONFIG(tag, "  BLE Service: %s", this->ble_service_.to_string().c_str()); \
  ESP_LOGCONFIG(tag, "  BLE Char TX: %s", this->ble_char_tx_.to_string().c_str()); \
  ESP_LOGCONFIG(tag, "  BLE Char RX: %s", this->ble_char_rx_.to_string().c_str()); \
  ESP_LOGCONFIG(tag, "  BLE Sec Act: %u", this->ble_sec_act_); \
  ESP_LOGCONFIG(tag, "  Persistent connection: %s", ONOFF(this->persistent_connection_));

class VPortBLENode : public ble_client::BLEClientNode {
 public:
  void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if,
                           esp_ble_gattc_cb_param_t *param) override;

  // TODO move to delegate?
  virtual void on_ble_ready() = 0;
  // TODO move to delegate?
  virtual bool on_ble_data(const uint8_t *data, uint16_t size) = 0;
  bool write_ble_data(const uint8_t *data, uint16_t size);

  void set_ble_service(const char *ble_service) {
    this->ble_service_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_service);
  }
  void set_ble_char_tx(const char *ble_char_tx) {
    this->ble_char_tx_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_char_tx);
  }
  void set_ble_char_rx(const char *ble_char_rx) {
    this->ble_char_rx_ = esp32_ble_tracker::ESPBTUUID::from_raw(ble_char_rx);
  }
  void set_ble_encryption(esp_ble_sec_act_t ble_sec_act) { this->ble_sec_act_ = ble_sec_act; }

  virtual bool ble_reg_for_notify() const { return true; }

  bool is_connected() const { return this->node_state == esp32_ble_tracker::ClientState::ESTABLISHED; }
  void connect() { this->parent()->set_enabled(true); }
  void disconnect() { this->parent()->set_enabled(false); }
  void schedule_disconnect(Component *component, uint32_t timeout);
  void cancel_disconnect(Component *component);

  void set_persistent_connection(bool persistent_connection) { this->persistent_connection_ = persistent_connection; }
  bool is_persistent_connection() const { return this->persistent_connection_; }
  void set_disable_scan(bool disable_scan) { this->disable_scan_ = disable_scan; }
#ifdef VPORT_BLE_ENABLE_QUEUE
  size_t get_max_queue_size() const { return 20; }
#endif
 protected:
  uint16_t char_rx_{};
  uint16_t char_tx_{};
  esp32_ble_tracker::ESPBTUUID ble_service_{};
  esp32_ble_tracker::ESPBTUUID ble_char_tx_{};
  esp32_ble_tracker::ESPBTUUID ble_char_rx_{};
  esp_ble_sec_act_t ble_sec_act_{esp_ble_sec_act_t::ESP_BLE_SEC_ENCRYPT_MITM};
  bool persistent_connection_{};
  bool disable_scan_{};
#ifdef VPORT_BLE_ENABLE_QUEUE
  std::vector<std::vector<uint8_t>> waited_;
  // TODO move to circular_buffer
  // etl::circular_buffer<etl::ivector<uint8_t>, 20> waited__;
#endif
  void write_queue();
  bool write_ble_data_(const uint8_t *data, uint16_t size) const;
  void on_ble_ready_();
};

template<typename frame_type> class VPortBLEComponentBase : public VPortComponent<frame_type>, public VPortBLENode {
 public:
  void schedule_disconnect(uint32_t timeout = 3000) { VPortBLENode::schedule_disconnect(this, timeout); }
  void cancel_disconnect() { VPortBLENode::cancel_disconnect(this); }

  void on_ble_ready() override {
    this->fire_ready();
    this->schedule_disconnect();
  }

  virtual bool should_disconnect(frame_type type) = 0;

  bool write_data(const uint8_t *data, size_t size) { return this->write_ble_data(data, size); }

  bool read_frame(frame_type type, const void *data, size_t size) {
    this->fire_frame(type, data, size);
    if (this->should_disconnect(type)) {
      this->cancel_disconnect();
      if (!this->is_persistent_connection()) {
        this->schedule_disconnect();
      }
    }
    return true;
  }
};

/// Requires following protocol class.
/// interface ProtocolClass {
///   using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
///   using reader_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
///  public:
///   reader_type reader{};
///   writer_type writer{};
///   bool write_frame(uint16_t type, const void *data, size_t size);
///   bool read_data(const uint8_t *data, size_t size);
///   const char *get_ble_service() const;
///   const char *get_ble_char_tx() const;
///   const char *get_ble_char_rx() const;
///   esp_ble_sec_act_t get_ble_encryption() const;
/// }
template<typename frame_type, class protocol_type, class base_type> class VPortBLEComponent : public base_type {
  static_assert(std::is_base_of<VPortBLEComponentBase<frame_type>, base_type>::value,
                "base_type is not derived from VPortBLEComponentBase");

  using this_type = VPortBLEComponent<frame_type, protocol_type, base_type>;
  using component_type = VPortBLEComponentBase<frame_type>;

 public:
  explicit VPortBLEComponent(protocol_type *protocol) : protocol_(protocol) {
    protocol_->reader.template set<component_type, &component_type::read_frame>(*this);
    protocol_->writer.template set<component_type, &component_type::write_data>(*this);

    this->set_ble_service(protocol->get_ble_service());
    this->set_ble_char_tx(protocol->get_ble_char_tx());
    this->set_ble_char_rx(protocol->get_ble_char_rx());
    if (protocol->get_ble_encryption() != 0) {
      this->set_ble_encryption(protocol->get_ble_encryption());
    }
  }

  bool on_ble_data(const uint8_t *data, uint16_t size) override { return this->protocol_->read_data(data, size); }

  // using writer_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
  // void setup_frame_writer(writer_type &writer) {
  //   writer.set<protocol_type, &protocol_type::write_frame>(*this->protocol_);
  // }

 protected:
  protocol_type *protocol_;
};

}  // namespace vport
}  // namespace esphome
#endif
