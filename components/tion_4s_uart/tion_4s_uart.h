#pragma once

#include "esphome/core/defines.h"
#include "esphome/components/vport/vport_uart.h"

#include "../tion/tion.h"
#include "../tion/tion_vport_uart.h"
#include "../tion-api/tion-api-4s.h"
#include "../tion-api/tion-api-uart-lt.h"

namespace esphome {
namespace tion {
/*
template<class io_t, class frame_spec_t>
class TionVPortUARTComponent : public vport::VPortUARTComponent<io_t, frame_spec_t, PollingComponent> {
  using super_t = vport::VPortUARTComponent<io_t, frame_spec_t, PollingComponent>;

 public:
  explicit TionVPortUARTComponent(io_t *io) : super_t(io) {
#ifdef USE_TION_HALF_DUPLEX
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    this->io_->set_on_frame(io_t::on_frame_type::template create<this_t, &this_t::on_frame_>(*this));
#endif
  }

  TionVPortType get_vport_type() const { return TionVPortType::VPORT_UART; }

#ifdef USE_TION_HALF_DUPLEX
  void write(const frame_spec_t &frame, size_t size) override {
    if (this->await_frame_) {
      auto data8 = reinterpret_cast<const uint8_t *>(&frame);
      auto datav = std::vector<uint8_t>(data8, data8 + size);
      this->defer([this, datav]() {
        auto frame = reinterpret_cast<const frame_spec_t *>(datav.data());
        if (this->await_frame_) {
          ESP_LOGW("tion_4s_uart", "prev frame is not recv, may lead to crash");
        }
        super_t::write(*frame, datav.size());
        arch_feed_wdt();
        yield();
      });
    } else {
      this->await_frame_ = true;
      super_t::write(frame, size);
      arch_feed_wdt();
      yield();
    }
  }

 protected:
  bool await_frame_{};
  void on_frame_(const frame_spec_t &frame, size_t size) {
    arch_feed_wdt();
    yield();
    this->await_frame_ = false;
    this->fire_frame(frame, size);
  }
#endif
};
*/
using Tion4sUartIO = TionUartIO<dentra::tion::TionUartProtocolLt>;

class Tion4sUartVPort : public TionVPortUARTComponent<Tion4sUartIO, Tion4sUartIO::frame_spec_type, PollingComponent> {
 public:
  explicit Tion4sUartVPort(Tion4sUartIO *io) : TionVPortUARTComponent(io){};

  void dump_config() override;
  void setup() override;
  void update() override { this->api_->send_heartbeat(); }

  void set_api(dentra::tion::TionApi4s *api) { this->api_ = api; }
  void set_state_type(uint16_t state_type) {}

 protected:
  void super_setup_() { TionVPortUARTComponent::setup(); }
  dentra::tion::TionApi4s *api_;
};

}  // namespace tion
}  // namespace esphome
