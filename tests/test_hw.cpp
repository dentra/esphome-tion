
#include <cstring>

#include "utils.h"
#include "../components/tion-api/log.h"
#include "../components/tion-api/crc.h"
#include "../components/tion-api/tion-api-uart.h"
#include "../components/tion-api/tion-api-ble-lt.h"
#include "../components/tion-api/tion-api-4s.h"

#include "test_hw.h"

#define HW_MAGIC ((uint8_t) 0x3A)

using namespace dentra::tion;

enum {
  FRAME_TYPE_STATE_SET = 0x3230,  // no save req
  FRAME_TYPE_STATE_RSP = 0x3231,
  FRAME_TYPE_STATE_REQ = 0x3232,
  FRAME_TYPE_STATE_SAV = 0x3234,  // save req

  FRAME_TYPE_DEV_STATUS_REQ = 0x3332,
  FRAME_TYPE_DEV_STATUS_RSP = 0x3331,

  FRAME_TYPE_TEST_REQ = 0x3132,
  FRAME_TYPE_TEST_RSP = 0x3131,

  FRAME_TYPE_TIMER_SET = 0x3430,
  FRAME_TYPE_TIMER_REQ = 0x3432,
  FRAME_TYPE_TIMER_RSP = 0x3431,

  FRAME_TYPE_TIMERS_STATE_REQ = 0x3532,
  FRAME_TYPE_TIMERS_STATE_RSP = 0x3531,

  FRAME_TYPE_TIME_SET = 0x3630,
  FRAME_TYPE_TIME_REQ = 0x3632,
  FRAME_TYPE_TIME_RSP = 0x3631,

  FRAME_TYPE_ERR_CNT_REQ = 0x3732,
  FRAME_TYPE_ERR_CNT_RSP = 0x3731,

  FRAME_TYPE_TEST_SET = 0x3830,  // FRAME_TYPE_CURR_TEST_SET
  FRAME_TYPE_CURR_TEST_REQ = 0x3832,
  FRAME_TYPE_CURR_TEST_RSP = 0x3831,

  FRAME_TYPE_TURBO_SET = 0x4130,
  FRAME_TYPE_TURBO_REQ = 0x4132,
  FRAME_TYPE_TURBO_RSP = 0x4131,

  FRAME_TYPE_HEARTBIT_REQ = 0x3932,  // every 3 sec
  FRAME_TYPE_HEARTBIT_RSP = 0x3931,
};

bool check(const dentra::tion::tion4s_state_t &ss, check_fn_t fn) {
  LOGD("  tion_hw_rsp_state_t (size=%u)", sizeof(ss));
  LOGD("    power_state    : %s", ONOFF(ss.flags.power_state));
  LOGD("    sound_state    : %s", ONOFF(ss.flags.sound_state));
  LOGD("    led_state      : %s", ONOFF(ss.flags.led_state));
  LOGD("    heater_state   : %s", ONOFF(ss.flags.heater_state));
  LOGD("    heater_mode    : %u", ss.flags.heater_mode);
  LOGD("    last_com_source: %s", ONOFF(ss.flags.last_com_source));
  LOGD("    filter_warnout : %s", ONOFF(ss.flags.filter_warnout));
  LOGD("    heater_present : %u", ss.flags.heater_present);
  LOGD("    ma             : %s", ONOFF(ss.flags.ma));
  LOGD("    ma_auto        : %s", ONOFF(ss.flags.ma_auto));
  LOGD("    active_timer   : %s", ONOFF(ss.flags.active_timer));
  LOGD("    reserved       : %02X", ss.flags.reserved);
  LOGD("    recirculation  : %s",
       ONOFF(ss.gate_position != dentra::tion::tion4s_state_t::GatePosition::GATE_POSITION_INFLOW));
  LOGD("    target_temp    : %d", ss.target_temperature);
  LOGD("    fan_speed      : %u", ss.fan_speed);
  LOGD("    current_temp   : %d", ss.current_temperature);
  LOGD("    outdoor_temp   : %d", ss.outdoor_temperature);
  LOGD("    pcb_ctl_temp   : %d", ss.pcb_ctl_temperature);
  LOGD("    pcb_pwr_temp   : %d", ss.pcb_pwr_temperature);
  LOGD("    cnt.work_time  : %08X", ss.counters.work_time);
  LOGD("    cnt.fan_time   : %08X", ss.counters.fan_time);
  LOGD("    cnt.filter_time: %08X", ss.counters.filter_time);
  LOGD("    cnt.airflow_cnt: %08X", ss.counters.airflow_counter_);
  LOGD("    errors         : %08X", ss.errors);
  LOGD("    max_fan_speed  : %u", ss.max_fan_speed);
  LOGD("    heater_var     : %u", ss.heater_var);
  return fn(&ss);
}

bool check(const tion_hw_set_state_t &ss, check_fn_t fn) {
  LOGD("  tion_hw_req_state_t");
  LOGD("    power_state    : %s", ONOFF(ss.power_state));
  LOGD("    sound_state    : %s", ONOFF(ss.sound_state));
  LOGD("    led_state      : %s", ONOFF(ss.led_state));
  LOGD("    heater_state   : %s", ONOFF(ss.heater_mode == 0));
  LOGD("    last_com_source: %s", ONOFF(ss.last_com_source));
  LOGD("    factory_reset  : %s", ONOFF(ss.factory_reset));
  LOGD("    error_reset    : %s", ONOFF(ss.error_reset));
  LOGD("    filter_reset   : %s", ONOFF(ss.filter_reset));
  LOGD("    ma             : %s", ONOFF(ss.ma));
  LOGD("    ma_auto        : %s", ONOFF(ss.ma_auto));
  LOGD("    reserved       : %02X", ss.reserved);
  LOGD("    recirculation  : %s",
       ONOFF(ss.gate_position != dentra::tion::tion4s_state_t::GatePosition::GATE_POSITION_INFLOW));
  LOGD("    target_temp    : %d", ss.target_temperature);
  LOGD("    fan_speed      : %u", ss.fan_speed);
  LOGD("    unknown        : %04X", ss.unknown);
  return fn(&ss);
}

bool check_cmd(uint16_t type, const void *data, size_t size, check_fn_t fn) {
  LOGD("checking commad %s", hexencode(data, size).c_str());
  LOGD("  command %04X, data: %s", type, hexencode(data, size).c_str());
  switch (type) {
    case FRAME_TYPE_STATE_SET: {
      if (size != sizeof(tion_hw_req_frame_state_t)) {
        LOGE("Incorrect state request data size: %u", size);
        return false;
      }
      auto frame = static_cast<const tion_hw_req_frame_state_t *>(data);
      LOGD("  request_id %u", frame->request_id);
      return check(frame->state, fn);
    }

    case FRAME_TYPE_STATE_RSP: {
      if (size != sizeof(tion_hw_rsp_frame_state_t)) {
        LOGE("Incorrect state request data size: %u", size);
        return false;
      }
      auto frame = static_cast<const tion_hw_rsp_frame_state_t *>(data);
      LOGD("  request_id %u", frame->request_id);
      return check(frame->state, fn);
    }

    case FRAME_TYPE_HEARTBIT_REQ: {
      if (size != 0) {
        LOGE("Incorrect heartbeat request data size: %u", size);
        return false;
      }
      break;
    }

    case FRAME_TYPE_HEARTBIT_RSP: {
      if (size != sizeof(tion_hw_rsp_heartbeat_t)) {
        LOGE("Incorrect state request data size: %u", size);
        return false;
      }
      auto frame = static_cast<const tion_hw_rsp_heartbeat_t *>(data);
      return frame->unknown00 == 0;
    }

    case FRAME_TYPE_DEV_STATUS_REQ: {
      if (size != 0) {
        LOGE("Incorrect device status request data size: %u", size);
        return false;
      }
      break;
    }

    default:
      LOGW("Unsupported command %04X", type);
  }
  return true;
}

bool check_packet(const std::vector<uint8_t> &raw, check_fn_t fn) {
  LOGD("checking packet %s", hexencode(raw.data(), raw.size()).c_str());
  auto packet = reinterpret_cast<const tion_hw_packet_t *>(raw.data());
  if (packet->magic != HW_MAGIC) {
    LOGD("invalid magic %02X", packet->magic);
    return false;
  }

  if (packet->size != raw.size()) {
    LOGD("invalid size %u", packet->size);
    return false;
  }

  if (crc16_ccitt_false(packet, packet->size) != 0) {
    uint16_t crc = __builtin_bswap16(*(uint16_t *) (raw.data() + raw.size() - sizeof(uint16_t)));
    LOGD("invalid crc %04X (expected: %04X)", crc, crc16_ccitt_false(raw.data(), raw.size() - sizeof(uint16_t)));
    return false;
  }

  return check_cmd(packet->type, packet->data, packet->size - sizeof(tion_hw_packet_t), fn);
}

class StringUart {
 public:
  StringUart(const char *data) {
    auto size = std::strlen(data);
    this->data_ = new char[size / 2];
    auto ptr = data;
    auto end = data + size;
    while (ptr < end) {
      if ((*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':')) {
        ptr++;
        continue;
      }
      auto byte = char2int(*ptr) << 4;
      ptr++;
      if (ptr < end) {
        byte += char2int(*ptr);
        this->data_[this->size_++] = byte;
        ptr++;
      }
    }
  }

  virtual ~StringUart() { delete this->data_; }

  bool read_array_(void *data, size_t size) {
    if (this->pos_ + size > this->size_) {
      return false;
    }
    std::memcpy(data, &this->data_[this->pos_], size);
    this->pos_ += size;
    return true;
  }

  int available_() const {
    int av = this->size_ - this->pos_;
    return av;
  }

  bool read_array(void *data, size_t size) {
    if (size > this->available()) {
      return false;
    }
    return this->read_array_(data, size);
  }

  int available() const {
    auto av = this->available_();
    return this->av_ < av ? this->av_ : av;
  }

  void inc_av() {
    if (this->av_ < this->available_()) {
      this->av_++;
    }
  }

  int read() {
    uint8_t ch;
    return this->read_byte(&ch) ? ch : -1;
  }
  bool read_byte(uint8_t *ch) { return this->available() > 0 ? this->read_array(ch, 1) : false; }
  bool peek_byte(uint8_t *data) {
    if (this->available()) {
      *data = this->data_[this->pos_];
      return true;
    }
    return false;
  }

 private:
  size_t size_{};
  size_t pos_{};
  char *data_;
  size_t av_ = 1;
};
/* TODO implement
class Tion4sHWApiTest : public TionApi4s {
 public:
  explicit Tion4sHWApiTest(TionFrameWriter *writer) : TionApi4s(writer) {}
  bool read_frame(uint16_t type, const void *data, size_t size) override {
    // switch (type) {
    //   default:
    //     LOGW("Unsupported command %04X", type);
    //     break;
    // }
    auto check_fn_ok = [](const void *data) { return true; };
    return check_cmd(type, data, size, check_fn_ok);
  }
};
*/

class StringUartIO : public StringUart, public TionUartReader {
 public:
  explicit StringUartIO(const char *data) : StringUart(data) {}
  int available() override { return StringUart::available(); }
  bool read_array(void *data, size_t size) override { return StringUart::read_array(data, size); }
};

/* TODO implement
class FakeUartReader : public StringUartIO, public TionUartProtocol {
 public:
  explicit FakeUartReader(const char *data) : StringUartIO(data) {}
  void set_api(TionFrameReader *api) { this->reader_ = api; }
  bool read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size) override {
    return this->reader_->read_frame(frame_type, frame_data, frame_data_size);
  }
  bool write_data(const uint8_t *data, size_t size) const override {
    printf("[FakeUartReader] write_data: %s\n", hexencode(data, size).c_str());
    return true;
  }

 protected:
  dentra::tion::TionFrameReader *reader_{};
};

bool test_long() {
  StringUart uart(test_data_long2);
  int ch = -1;
  while ((ch = uart.read()) != -1) {
    if (ch == HW_MAGIC) {
      printf("\n");
    }
    printf("%02X", ch);
  }
  printf("\n");
  return true;
}

bool test_long2() {
  FakeUartReader ur(test_data_long);
  Tion4sHWApiTest api(&ur);
  ur.set_api(&api);
  ur.read_uart_data(&ur);
  printf("done\n");
  return true;
}
*/

void start();
bool test_hw(bool print) {
  bool res = true;
  /*
    auto check_fn_ok = [](const void *data) { return true; };

    // invalid magic
    test_check(res, check_packet(from_hex("AA 07 00 32 39 CE EC"), check_fn_ok), false);
    // invalid size
    test_check(res, check_packet(from_hex("3A AA 00 32 39 CE EC"), check_fn_ok), false);
    // invalid crc (error in type)
    test_check(res, check_packet(from_hex("3A 07 00 32 AA CE EC"), check_fn_ok), false);

    for (auto td : test_data) {
      auto type = td.type == hw_test_data_t::SET   ? "SET"
                  : td.type == hw_test_data_t::REQ ? "REQ"
                  : td.type == hw_test_data_t::RSP ? "RSP"
                                                   : "UNK";

      LOGI("%s: %s", type, td.name);
      test_check(res, check_packet(from_hex(td.data), td.func ? td.func : check_fn_ok), true);
    }

    test_long();
    test_long2();

    // Tion4sHWApiTest api;
    // api.request_dev_status();
  */
  start();

  return res;
}

#include "test_vport.h"

class StringUartVPort : public TionVPortComponent<TionUartProtocol> {
 public:
  StringUartVPort(const char *data, TionUartProtocol *protocol) : TionVPortComponent(protocol), io_(data) {}
  void loop() override {
    while (this->io_.available_() > 0) {
      this->protocol_->read_uart_data(&this->io_);
      this->io_.inc_av();
      this->io_.inc_av();
    }
  }

 protected:
  StringUartIO io_;
};

template<class VPortT, class ProtocolT> class TestTion4s {
  using this_type = TestTion4s<VPortT, ProtocolT>;

 public:
  explicit TestTion4s(VPortT *vport, ProtocolT *protocol) {
    vport->on_frame.template set<this_type, &this_type::on_frame>(*this);

    this->api_.writer.set<ProtocolT, &ProtocolT::write_frame>(*protocol);
    this->api_.on_state.set<this_type, &this_type::on_state>(*this);
    this->api_.on_heartbeat.set<this_type, &this_type::on_heartbeat>(*this);
  }

  void on_state(const tion4s_state_t &state, uint32_t request_id) {}

  void on_heartbeat(uint8_t unknown) { this->api_.send_heartbeat(); }

  bool on_frame(uint16_t type, const void *data, size_t size) {
    printf("[TestTion4s] Reading type: %04X, data: %s\n", type, hexencode(data, size).c_str());
    return this->api_.read_frame(type, data, size);
  }

  void run() {
    this->api_.send_heartbeat();
    this->api_.request_turbo();
  }

 protected:
  TionApi4s api_;
};

void start_uart() {
  TionUartProtocol uart_protocol;
  StringUartVPort uart_vport("3A 0800 3139 00 E82B", &uart_protocol);
  TestTion4s<StringUartVPort, TionUartProtocol> test(&uart_vport, &uart_protocol);
  uart_vport.loop();
  test.run();
}

void start_ble() {
  TionBleLtProtocol ble_protocol;
  StringBleVPort ble_vport("80 0D00 3AAD 3139 01000000 00 184B", &ble_protocol);
  TestTion4s<StringBleVPort, TionBleLtProtocol> test(&ble_vport, &ble_protocol);
  ble_vport.loop();
  test.run();
}

#include "etl/circular_buffer.h"
#include "etl/vector.h"
#include "etl/vector.h"

void start() {
  start_uart();
  start_ble();

  etl::circular_buffer<std::vector<uint8_t>, 5> cb;
  for (int i = 0; i < 10; i++) {
    cb.push(std::vector<uint8_t>({1, 2, 3}));
  }

  printf("* %s\n", hexencode(cb.front().data(), cb.front().size()).c_str());
  cb.pop();

  for (auto x : cb) {
    printf("%s\n", hexencode(x.data(), x.size()).c_str());
  }
}
