#include "esphome/core/log.h"

#include "../tion-api/tion-api-3s-internal.h"

#include "tion_3s_proxy.h"

namespace esphome {
namespace tion_3s_proxy {

static const char *const TAG = "tion_3s_proxy";

// convert 0x013D to 0x01 (FRAME_MAGIC_REQ=3D)
#define FRAME_REQ_TO_CMD(req) ((req) >> 8)
// convert 0x50B3 to 0x05 (FRAME_MAGIC_RSP=B3)
#define FRAME_RSP_TO_CMD(rsp) ((rsp) >> 12)

using dentra::tion_3s::FRAME_TYPE_SRV_MODE_SET;

void Tion3sApiProxy::read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) {
  // сюда прилетают команды типа RSP, для прокси RSP это RX
  auto cmd = FRAME_RSP_TO_CMD(frame_type);
  // фильтруем команды на которые были запросы.
  // FRAME_TYPE_SRV_MODE_SET прилетает от бризера без специального запроса,
  // после ручного нажатие кнопки сопряжения
  if (cmd != FRAME_TYPE_SRV_MODE_SET && this->ble_->get_last_cmd() != cmd) {
    return;
  }
  auto *data8 = static_cast<const uint8_t *>(frame_data);
  ESP_LOGD(TAG, "RX (%04X): %s", frame_type, format_hex_pretty(data8, frame_data_size).c_str());
  this->ble_->write_frame(frame_type, frame_data, frame_data_size);
  this->ble_->reset_last_cmd();
}

void Tion3sBleProxy::on_frame_(const frame_spec_type &frame, size_t size) {
  // сюда прилетают команды типа REQ, для прокси RSP это TX
  const auto frame_data_size = size - frame_spec_type::head_size();
  ESP_LOGD(TAG, "TX (%04X): %s", frame.type, format_hex_pretty(frame.data, frame_data_size).c_str());
  this->api_->write_frame(frame.type, frame.data, frame_data_size);
  // сохраняем команду для дальнейшей фильтрации
  this->last_cmd_ = FRAME_REQ_TO_CMD(frame.type);
}

void Tion3sBleProxy::dump_config() { ESP_LOGCONFIG(TAG, "Tion 3S Proxy"); }

}  // namespace tion_3s_proxy
}  // namespace esphome
