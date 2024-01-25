#include "esphome/core/log.h"

#include "../tion-api/tion-api-o2-internal.h"

#include "tion_o2_proxy.h"

namespace esphome {
namespace tion_o2_proxy {

static const char *const TAG = "tion_o2_proxy";

#define FRAME_REQ_TO_CMD(req) ((req))
#define FRAME_RSP_TO_CMD(rsp) ((rsp) >> 4)

void TionO2ApiProxy::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  auto *data8 = static_cast<const uint8_t *>(frame_data);
  ESP_LOGD(TAG, "RX [%02X]:%s", frame_type, format_hex_pretty(data8, frame_data_size).c_str());
  this->parent_->tx_->write_frame(frame_type, frame_data, frame_data_size);
}

void TionO2Proxy::on_frame_(const dentra::tion_o2::TionO2UartProtocol::frame_spec_type &frame, size_t size) {
  const auto frame_data_size = size - dentra::tion_o2::TionO2UartProtocol::frame_spec_type::head_size();
  ESP_LOGD(TAG, "TX [%02X]:%s", frame.type, format_hex_pretty(frame.data, frame_data_size).c_str());
  this->rx_->write_frame(frame.type, frame.data, frame_data_size);
}

void TionO2Proxy::dump_config() { ESP_LOGCONFIG(TAG, "Tion O2 Proxy"); }

}  // namespace tion_o2_proxy
}  // namespace esphome
