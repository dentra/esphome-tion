#include "esphome/core/log.h"

#include "../tion-api/tion-api-3s-internal.h"

#include "tion_3s_proxy.h"

namespace esphome {
namespace tion_3s_proxy {

static const char *const TAG = "tion_3s_proxy";

#define FRAME_REQ_TO_CMD(req) ((req) >> 8)
#define FRAME_RSP_TO_CMD(rsp) ((rsp) >> 12)

void Tion3sApiProxy::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  auto cmd = FRAME_RSP_TO_CMD(frame_type);
  if (cmd != dentra::tion_3s::FRAME_TYPE_SRV_MODE_SET && this->parent_->last_cmd_ != cmd) {
    return;
  }
  auto *data8 = static_cast<const uint8_t *>(frame_data);
  ESP_LOGD(TAG, "RX (%04X): %s", frame_type, format_hex_pretty(data8, frame_data_size).c_str());
  this->parent_->tx_->write_frame(frame_type, frame_data, frame_data_size);
  this->parent_->last_cmd_ = 0;
}

void Tion3sProxy::on_frame_(const dentra::tion::Tion3sUartProtocol::frame_spec_type &frame, size_t size) {
  const auto frame_data_size = size - dentra::tion::Tion3sUartProtocol::frame_spec_type::head_size();
  ESP_LOGD(TAG, "TX (%04X): %s", frame.type, format_hex_pretty(frame.data, frame_data_size).c_str());
  this->rx_->write_frame(frame.type, frame.data, frame_data_size);
  this->last_cmd_ = FRAME_REQ_TO_CMD(frame.type);
}

void Tion3sProxy::dump_config() { ESP_LOGCONFIG(TAG, "Tion 3S Proxy"); }

}  // namespace tion_3s_proxy
}  // namespace esphome
