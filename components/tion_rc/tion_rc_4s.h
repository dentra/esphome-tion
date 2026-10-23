#pragma once

#include "tion_rc.h"
#include "../tion-api/tion-api-ble-lt.h"

namespace esphome {
namespace tion_rc {

class Tion4sRCBleProtocol : public dentra::tion::TionLtBleProtocol {
 public:
  Tion4sRCBleProtocol() : dentra::tion::TionLtBleProtocol(false) {}
};

class Tion4sRC : public TionRCControlImpl<Tion4sRCBleProtocol> {
 public:
  Tion4sRC(dentra::tion::TionApiBase *api) : TionRCControlImpl(api) {}
  void adv(bool pair) override;
  // real 4S (nRF52) advertises with a random static address
  bool use_random_address() const override { return true; }
  void on_state(const dentra::tion::TionState &st) override;
  void on_frame(uint16_t type, const uint8_t *data, size_t size) override;
#ifdef USE_TION_RC_EMU
  void loop() override;
#endif

 protected:
#ifdef USE_TION_RC_EMU
  // Built-in 4S emulation (no tion_id): state is kept here and changed by the rules of real firmware 0x04C7.
  dentra::tion_4s::tion4s_state_t emu_{};
  uint32_t emu_req_id_{};
  uint32_t emu_tick_{};
  uint32_t emu_frames_{};
  uint32_t emu_sets_{};
  uint16_t emu_turbo_left_{};
  uint8_t emu_turbo_fan_{};
  int64_t emu_time_{};
  void emu_reset_();
  void emu_on_frame_(uint16_t type, const uint8_t *data, size_t size);
  void emu_send_state_(uint32_t request_id);
  void emu_send_turbo_(uint32_t request_id);
#endif
#ifdef TION_UPDATE_EMU
  bool is_update_{};
  uint32_t fw_size_{};
  uint32_t fw_load_{};
  uint16_t fw_crc_{};
#endif
};

}  // namespace tion_rc
}  // namespace esphome
