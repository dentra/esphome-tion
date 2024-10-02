
#include <cstring>
#include "etl/circular_buffer.h"
#include "etl/vector.h"

#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/crc.h"
#include "../components/tion-api/tion-api-uart-4s.h"
#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-4s.h"

#include "esphome/components/vport/vport_uart.h"
#include "esphome/components/vport/vport_ble.h"
#include "test_api.h"
#include "test_vport.h"
#include "test_hw.h"

DEFINE_TAG;

#define HW_MAGIC ((uint8_t) 0x3A)

using namespace esphome;

using dentra::tion_4s::tion4s_state_t;
using dentra::tion::TionGatePosition;
using dentra::tion::tion_dev_info_t;
using dentra::tion::TionState;
using Tion4sApi = dentra::tion_4s::Tion4sApi;

bool check(const tion4s_state_t &ss, check_fn_t fn) {
  ESP_LOGD(TAG, "  tion_hw_rsp_state_t (size=%zu)", sizeof(ss));
  ESP_LOGD(TAG, "    power_state    : %s", ONOFF(ss.power_state));
  ESP_LOGD(TAG, "    sound_state    : %s", ONOFF(ss.sound_state));
  ESP_LOGD(TAG, "    led_state      : %s", ONOFF(ss.led_state));
  ESP_LOGD(TAG, "    heater_state   : %s", ONOFF(ss.heater_state));
  ESP_LOGD(TAG, "    heater_mode    : %u", ss.heater_mode);
  ESP_LOGD(TAG, "    last_com_source: %u", uint8_t(ss.comm_source));
  ESP_LOGD(TAG, "    filter_warnout : %s", ONOFF(ss.filter_state));
  ESP_LOGD(TAG, "    heater_present : %u", ss.heater_present);
  ESP_LOGD(TAG, "    ma             : %s", ONOFF(ss.ma_connected));
  ESP_LOGD(TAG, "    ma_auto        : %s", ONOFF(ss.ma_auto));
  ESP_LOGD(TAG, "    active_timer   : %s", ONOFF(ss.active_timer));
  ESP_LOGD(TAG, "    reserved       : %02X", ss.reserved);
  ESP_LOGD(TAG, "    recirculation  : %s",
           ONOFF(ss.gate_position != tion4s_state_t::GatePosition::GATE_POSITION_OUTDOOR));
  ESP_LOGD(TAG, "    target_temp    : %d", ss.target_temperature);
  ESP_LOGD(TAG, "    fan_speed      : %u", ss.fan_speed);
  ESP_LOGD(TAG, "    current_temp   : %d", ss.current_temperature);
  ESP_LOGD(TAG, "    outdoor_temp   : %d", ss.outdoor_temperature);
  ESP_LOGD(TAG, "    pcb_ctl_temp   : %d", ss.pcb_ctl_temperature);
  ESP_LOGD(TAG, "    pcb_pwr_temp   : %d", ss.pcb_pwr_temperature);
  ESP_LOGD(TAG, "    cnt.work_time  : %08X", ss.counters.work_time);
  ESP_LOGD(TAG, "    cnt.fan_time   : %08X", ss.counters.fan_time);
  ESP_LOGD(TAG, "    cnt.filter_time: %08X", ss.counters.filter_time);
  ESP_LOGD(TAG, "    cnt.airflow_cnt: %08X", ss.counters.airflow_counter);
  ESP_LOGD(TAG, "    errors         : %08X", ss.errors);
  ESP_LOGD(TAG, "    max_fan_speed  : %u", ss.max_fan_speed);
  ESP_LOGD(TAG, "    heater_var     : %u", ss.heater_var);
  return fn(&ss);
}

bool check(const tion_hw_set_state_t &ss, check_fn_t fn) {
  ESP_LOGD(TAG, "  tion_hw_req_state_t");
  ESP_LOGD(TAG, "    power_state    : %s", ONOFF(ss.power_state));
  ESP_LOGD(TAG, "    sound_state    : %s", ONOFF(ss.sound_state));
  ESP_LOGD(TAG, "    led_state      : %s", ONOFF(ss.led_state));
  ESP_LOGD(TAG, "    heater_state   : %s", ONOFF(ss.heater_mode == 0));
  ESP_LOGD(TAG, "    last_com_source: %u", uint8_t(ss.last_com_source));
  ESP_LOGD(TAG, "    factory_reset  : %s", ONOFF(ss.factory_reset));
  ESP_LOGD(TAG, "    error_reset    : %s", ONOFF(ss.error_reset));
  ESP_LOGD(TAG, "    filter_reset   : %s", ONOFF(ss.filter_reset));
  ESP_LOGD(TAG, "    ma             : %s", ONOFF(ss.ma));
  ESP_LOGD(TAG, "    ma_auto        : %s", ONOFF(ss.ma_auto));
  ESP_LOGD(TAG, "    reserved       : %02X", ss.reserved);
  ESP_LOGD(TAG, "    recirculation  : %s",
           ONOFF(ss.gate_position != tion4s_state_t::GatePosition::GATE_POSITION_OUTDOOR));
  ESP_LOGD(TAG, "    target_temp    : %d", ss.target_temperature);
  ESP_LOGD(TAG, "    fan_speed      : %u", ss.fan_speed);
  ESP_LOGD(TAG, "    unknown        : %04X", ss.unknown);
  return fn(&ss);
}

bool check_cmd(uint16_t type, const void *data, size_t size, check_fn_t fn) {
  ESP_LOGD(TAG, "checking commad %s", hexencode_cstr(data, size));
  ESP_LOGD(TAG, "  command %04X, data: %s", type, hexencode_cstr(data, size));
  switch (type) {
    case dentra::tion_4s::FRAME_TYPE_STATE_SET: {
      if (size != sizeof(tion_hw_req_frame_state_t)) {
        ESP_LOGE(TAG, "Incorrect state request data size: %zu", size);
        return false;
      }
      const auto *frame = static_cast<const tion_hw_req_frame_state_t *>(data);
      ESP_LOGD(TAG, "  request_id %u", frame->request_id);
      return check(frame->state, fn);
    }

    case dentra::tion_4s::FRAME_TYPE_STATE_RSP: {
      if (size != sizeof(tion_hw_rsp_frame_state_t)) {
        ESP_LOGE(TAG, "Incorrect state request data size: %zu", size);
        return false;
      }
      const auto *frame = static_cast<const tion_hw_rsp_frame_state_t *>(data);
      ESP_LOGD(TAG, "  request_id %u", frame->request_id);
      return check(frame->state, fn);
    }

    case dentra::tion_4s::FRAME_TYPE_HEARTBEAT_REQ: {
      if (size != 0) {
        ESP_LOGE(TAG, "Incorrect heartbeat request data size: %zu", size);
        return false;
      }
      break;
    }

    case dentra::tion_4s::FRAME_TYPE_HEARTBEAT_RSP: {
      if (size != sizeof(tion_hw_rsp_heartbeat_t)) {
        ESP_LOGE(TAG, "Incorrect state request data size: %zu", size);
        return false;
      }
      const auto *frame = static_cast<const tion_hw_rsp_heartbeat_t *>(data);
      return frame->unknown00 == 0;
    }

    case dentra::tion_4s::FRAME_TYPE_DEV_INFO_REQ: {
      if (size != 0) {
        ESP_LOGE(TAG, "Incorrect device info request data size: %zu", size);
        return false;
      }
      break;
    }

    default:
      ESP_LOGW(TAG, "Unsupported command %04X", type);
  }
  return true;
}

bool check_packet(const std::string &hex, check_fn_t fn) {
  auto raw = cloak::from_hex(hex);
  ESP_LOGD(TAG, "checking packet %s", hexencode_cstr(raw));
  auto packet = reinterpret_cast<const tion_hw_packet_t *>(raw.data());
  if (packet->magic != HW_MAGIC) {
    ESP_LOGD(TAG, "invalid magic %02X", packet->magic);
    return false;
  }

  if (packet->size != raw.size()) {
    ESP_LOGD(TAG, "invalid size %u", packet->size);
    return false;
  }

  if (dentra::tion::crc16_ccitt_false_ffff(packet, packet->size) != 0) {
    uint16_t crc = __builtin_bswap16(*(uint16_t *) (raw.data() + raw.size() - sizeof(uint16_t)));
    ESP_LOGD(TAG, "invalid crc %04X (expected: %04X)", crc,
             dentra::tion::crc16_ccitt_false_ffff(raw.data(), raw.size() - sizeof(uint16_t)));
    return false;
  }

  return check_cmd(packet->type, packet->data, packet->size - sizeof(tion_hw_packet_t), fn);
}

bool test_hw_protocol() {
  bool res = true;

  auto check_fn_ok = [](const void *data) { return true; };

  res &= cloak::check_data("invalid magic", check_packet("AA 07 00 32 39 CE EC", check_fn_ok), false);
  res &= cloak::check_data("invalid size", check_packet("3A AA 00 32 39 CE EC", check_fn_ok), false);
  res &= cloak::check_data("invalid crc (error in type)", check_packet("3A 07 00 32 AA CE EC", check_fn_ok), false);

  for (auto td : test_data) {
    auto type = td.type == hw_test_data_t::SET   ? "SET"
                : td.type == hw_test_data_t::REQ ? "REQ"
                : td.type == hw_test_data_t::RSP ? "RSP"
                                                 : "UNK";
    res &= cloak::check_data(str_sprintf("%s: %s", type, td.name),
                             check_packet(td.data, td.func ? td.func : check_fn_ok), true);
  }

  return res;
}

using Tion4sUartVPortApiTest = tion::TionVPortApi<Tion4sUartIOTest::frame_spec_type, Tion4sApi>;
using Tion4sBleVPortApiTest = tion::TionVPortApi<Tion4sBleIOTest::frame_spec_type, Tion4sApi>;
using Tion4sUartVPortTest = vport::VPortUARTComponent<Tion4sUartIOTest, Tion4sUartIOTest::frame_spec_type>;
using TionLtBleVPortTest = vport::VPortBLEComponent<Tion4sBleIOTest, Tion4sBleIOTest::frame_spec_type>;

class Tion4sCompTest : public TionComponentTest<Tion4sApi> {
 public:
  Tion4sCompTest(Tion4sApi *api) : TionComponentTest(api) {
    using this_t = typename std::remove_pointer_t<decltype(this)>;
    api->on_state_fn.set<this_t, &this_t::on_state>(*this);
    api->on_heartbeat_fn.set<this_t, &this_t::on_heartbeat>(*this);
  }
  void on_state(const TionState &state, uint32_t request_id) {}
  void on_heartbeat(uint8_t work_mode) { this->api_->send_heartbeat(); }
};

bool test_hw_uart() {
  bool res = true;

  uart::UARTComponent uart("3A 0800 3139 00 E82B");
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPortTest vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sCompTest comp(&api);

  cloak::setup_and_loop({&vport});

  vport.call_loop();

  cloak::check_data("on_heartbeat", uart, "3A.07.00.32.39.CE.EC");

  api.send_heartbeat();
  vport.call_loop();
  cloak::check_data("send_heartbeat", uart, "3A.07.00.32.39.CE.EC");

  api.request_state();
  vport.call_loop();
  cloak::check_data("request_state", uart, "3A.07.00.32.32.7F.87");

  api.request_time(0);
  vport.call_loop();
  cloak::check_data("request_time", uart, "3A.0B.00.32.36.00.00.00.00.B0.E0");

  return res;
}

bool test_hw_ble() {
  bool res = true;

  ble_client::BLEClient client;
  client.set_address(0x112233445566);
  client.connect();

  Tion4sBleIOTest io(&client);
  TionLtBleVPortTest vport(&io);
  Tion4sBleVPortApiTest api(&vport);
  Tion4sCompTest comp(&api);

  cloak::setup_and_loop({&vport});

  vport.set_persistent_connection(true);
  client.enabled = true;
  io.node_state = esphome::esp32_ble_tracker::ClientState::ESTABLISHED;

  io.test_data_push(cloak::from_hex("80 0D00 3AAD 3139 01000000 00 184B"));
  vport.call_loop();
  res &= cloak::check_data("on_heartbeat", io, "80.0C.00.3A.AD.32.39.01.00.00.00.88.08");

  api.send_heartbeat();
  vport.call_loop();
  res &= cloak::check_data("send_heartbeat", io, "80.0C.00.3A.AD.32.39.01.00.00.00.88.08");

  api.request_state();  // request_state alsо requests dev_info first
  vport.call_loop();
  res &= cloak::check_data("request_dev_info", io, "80.0C.00.3A.AD.32.33.01.00.00.00.CE.A6");
  vport.call_loop();
  res &= cloak::check_data("request_state", io, "80.0C.00.3A.AD.32.32.01.00.00.00.64.F7");

  api.request_time(0);
  vport.call_loop();
  res &= cloak::check_data("request_time", io, "80.10.00.3A.AD.32.36.01.00.00.00.00.00.00.00.B9.37");

  io.connect();

  return res;
}

using Tion4sUartComponentTest = TionComponentTest<Tion4sUartVPortApiTest>;

bool test_hw_uart_on_ready() {
  bool res = true;

  uart::UARTComponent uart;
  Tion4sUartIOTest io(&uart);
  Tion4sUartVPortTest vport(&io);
  Tion4sUartVPortApiTest api(&vport);
  Tion4sUartComponentTest comp(&api);

  res &= cloak::check_data("not ready", comp.is_ready(), false);

  vport.call_setup();

  res &= cloak::check_data("is ready", comp.is_ready(), true);

  return res;
}

std::string hex_sanitize(const std::string &str) {
  std::string out;
  std::copy_if(str.begin(), str.end(), std::back_inserter(out), [](const char &c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  });
  return out;
}
bool parse_mac(std::string mac, uint64_t *result) {
  if (mac.length() < 12) {
    return false;
  }
  mac = hex_sanitize(mac);
  if (mac.length() != 12) {
    return false;
  }
  if (parse_hex(mac.c_str(), mac.length(), reinterpret_cast<uint8_t *>(result), sizeof(*result)) == 0) {
    return false;
  }
  *result = byteswap(*result) & 0xFFFFFFFFFFFFULL;
  return true;
}

bool test_111() {
  // 11:22:33:44:55:FF
  // 0x1122334455FFULL
  printf("%llx\n", 0x1122334455FFULL);
  uint64_t mac = 0xeeeeeeeeeeeeeeeeULL;
  parse_mac("11:22:33:44:55:FF", &mac);
  printf("%llx\n", mac);
  return true;
}

REGISTER_TEST(test_hw_protocol);
REGISTER_TEST(test_hw_uart);
REGISTER_TEST(test_hw_ble);
REGISTER_TEST(test_hw_uart_on_ready);
REGISTER_TEST(test_111);
