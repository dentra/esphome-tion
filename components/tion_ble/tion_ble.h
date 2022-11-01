#pragma once

#include "../vport/vport_ble.h"
#include "../tion/tion.h"
#include "../tion-api/tion-api-ble-lt.h"
#include "../tion-api/tion-api-ble.h"

namespace esphome {
namespace tion {

class TionBLEVPortBase : public vport::VPortBLEComponentBase<uint16_t> {
  using parent_type = vport::VPortBLEComponentBase<uint16_t>;

 public:
  void dump_config() override;
  void update() override;

  bool should_disconnect(uint16_t type) override { return type != 0 && this->state_type_ == type; }

  void set_state_type(uint16_t state_type) { this->state_type_ = state_type; }
  void set_state_timeout(uint32_t state_timeout) { this->state_timeout_ = state_timeout; }

  TionVPortType get_vport_type() const { return TionVPortType::VPORT_UART; }

  void set_cc(TionClimateComponentBase *cc) { this->cc_ = cc; }

  bool write_data(const uint8_t *data, size_t size) { return parent_type::write_data(data, size); }
  bool read_frame(uint16_t type, const void *data, size_t size) { return parent_type::read_frame(type, data, size); }

 protected:
  uint32_t state_timeout_{};
  uint16_t state_type_{};
  TionClimateComponentBase *cc_{};
  void dump_settings(const char *TAG);
};

template<class protocol_type, class base_type>
class TionBLEVPortT : public vport::VPortBLEComponent<uint16_t, protocol_type, base_type> {
  static_assert(std::is_base_of<TionBLEVPortBase, base_type>::value, "base_type is not derived from TionBLEVPortBase");

 public:
  explicit TionBLEVPortT(protocol_type *protocol)
      : vport::VPortBLEComponent<uint16_t, protocol_type, base_type>(protocol) {}

  // TODO move to python?
  void register_api_writer(dentra::tion::TionApiBaseWriter *api) {
    // this->setup_frame_writer(api->writer);
    api->writer.set<protocol_type, &protocol_type::write_frame>(*this->protocol_);
  }
};

template<class protocol_type> class TionBLEVPort final : public TionBLEVPortT<protocol_type, TionBLEVPortBase> {
 public:
  explicit TionBLEVPort(protocol_type *protocol) : TionBLEVPortT<protocol_type, TionBLEVPortBase>(protocol) {}
};

class VPortTionBleLtProtocol final : public dentra::tion::TionBleProtocol<dentra::tion::TionBleLtProtocol> {
 public:
  esp_ble_sec_act_t get_ble_encryption() const { return static_cast<esp_ble_sec_act_t>(0); }
  bool write_frame(uint16_t type, const void *data, size_t size) {
    return dentra::tion::TionBleLtProtocol::write_frame(type, data, size);
  }
};

}  // namespace tion
}  // namespace esphome
