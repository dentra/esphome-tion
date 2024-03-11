#pragma once

#include <cstddef>
#include <map>
#include <vector>
#include <type_traits>

#include <etl/delegate.h>

#include "utils.h"

namespace dentra {
namespace tion {

#pragma pack(push, 1)

enum class CommSource : uint8_t { AUTO = 0, USER = 1 };

// NOLINTNEXTLINE(readability-identifier-naming)
struct tion_dev_info_t {
  // NOLINTNEXTLINE(readability-identifier-naming)
  enum work_mode_t : uint8_t {
    UNKNOWN = 0,
    // обычный режим работы
    NORMAL = 1,
    // бризер находится в режиме обновления
    UPDATE = 2,
  } work_mode;
  // NOLINTNEXTLINE(readability-identifier-naming)
  enum device_type_t : uint32_t {
    BRO2 = 1,
    BR3S = 2,
    // Tion IQ 200
    IQ200 = 0x8001,
    // Tion Lite
    BRLT = 0x8002,
    // Tion 4S
    BR4S = 0x8003,
  } device_type;
  uint16_t firmware_version;
  uint16_t hardware_version;
  uint8_t reserved[16];
};

// NOLINTNEXTLINE(readability-identifier-naming)
template<size_t AK> struct tion_state_counters_t {
  // Motor time counter in seconds. power_up_time
  uint32_t work_time;
  // Electronics time count in seconds.
  uint32_t fan_time;
  // Filter time counter in seconds.
  uint32_t filter_time;
  // Airflow counter, m3=airflow_counter * 15.0 / 3600.0. - 4S
  //                  m3=airflow_counter * 10.0 / 3600.0. - Lite
  uint32_t airflow_counter;
  // Calculated airflow in m3.
  float airflow() const { return this->airflow_mult(this->airflow_counter) / 3600.0f; }
  // Calculated filter days left in days
  uint32_t filter_time_left_d() const { return this->filter_time / (24 * 3600); }
  // Calculated work time in days.
  uint32_t work_time_days() const { return this->work_time / (24 * 3600); }
  // Calculate airflow in m3/h.
  constexpr float airflow_mult(float counter) const { return counter * this->airflow_k(); }
  constexpr float airflow_k() const { return AK; }
};

#pragma pack(pop)

struct TionTraits {
  struct {
    bool initialized : 1;
    bool supports_led_state : 1;
    bool supports_sound_state : 1;
    // supports set gate position. at least outdoor/indoor
    bool supports_gate_position_change : 1;
    // supports set gate position. outdoor/indoor/mixed
    bool supports_gate_position_change_mixed : 1;
    bool supports_heater_var : 1;
    bool supports_work_time : 1;
    bool supports_fan_time : 1;
    bool supports_airflow_counter : 1;
    bool supports_gate_error : 1;
    bool supports_pcb_ctl_temperatire : 1;
    bool supports_pcb_pwr_temperature : 1;
    // true means antifrize must be supported manually
    bool supports_antifrize : 1;
    // true means native boost support
    bool supports_boost : 1;
    bool supports_reset_filter : 1;
  };

  using ErrorsDecoderPtr = std::add_pointer_t<std::string(uint32_t errors)>;
  ErrorsDecoderPtr errors_decoder{};

  // Время работы режима "Турбо" в секундах.
  uint16_t boost_time;
  // <0 - текущий режим работы, =0 - обогрев выключен, >0 - обогрев включен.
  int8_t boost_heater_state;
  // 0 - используем текущую целевую температуру.
  int8_t boost_target_temperature;

  // Максимальная скорость вентиляции.
  uint8_t max_fan_speed;
  // Минимальная целевая темперетура.
  int8_t min_target_temperature;
  // Максимальная целевая темперетура.
  int8_t max_target_temperature;

  // Мощность тэна в Вт * 0.1. Например, для 3S, значение будет 145.
  // Может быть 0, если теэн отсутсвует, актуально для моделей с индексом ECO.
  // 4s: 1000 и 1400, по паспорту 1000 и 800 (EU)
  // lt: 1000, по паспорту 850
  // 3s: 1450
  // o2: 1450
  uint8_t max_heater_power;
  // Потребление энергии без обогреватлея для всех скоростей в Вт * 100,
  // включая 0 - режим ожидания (standby), 1 - первая скорость и т.д.
  uint16_t max_fan_power[7];

  uint16_t get_max_heater_power() const { return this->max_heater_power * 10u; }
  float get_max_fan_power(size_t index) const { return this->max_fan_power[index] / 100.0f; }
};

enum class TionGatePosition : uint8_t {
  OUTDOOR = 0,
  INDOOR = 1,
  MIXED = 2,
  UNKNOWN = 0xF,
  OPENED = OUTDOOR,
  CLOSED = INDOOR,
  NONE = UNKNOWN,
};

class TionState {
 public:
  struct {
    // Состояние вкл/выкл.
    bool power_state : 1;
    // Состояние обогревателя вкл/выкл.
    bool heater_state : 1;
    // Состояние звуковых оповещений.
    bool sound_state : 1;
    // Состояние световых оповещений.
    bool led_state : 1;
    // Автоматичекское управление.
    bool auto_state : 1;
    // Предупреждение о необходимости замены фильтра.
    bool filter_state : 1;
    // Ошибка заслонки.
    bool gate_error_state : 1;
    // Источник изменений.
    CommSource comm_source : 1;
  };

  struct {
    bool reserved : 1;
    // Скорость вентиляции.
    // 4s, 3s, lt: [1-6]
    // o2: [1-4]
    uint8_t fan_speed : 3;
    // Позиция заслонки. 0 - закрыто, 1 - открыто, 2 - открыто частично
    // 4s: 0 - рециркуляция, 1 - забор воздуха
    // lt: 0 - закрыто, 1 - открыто
    // 3s: 0 - рециркуляция, 1 - забор воздуха, 2 - смешанный режим
    // o2: 0 - закрыто, 1 - открыто
    TionGatePosition gate_position : 4;
  };

  // Температура до нагревателя.
  int8_t outdoor_temperature;
  // Температура после нагревателя.
  int8_t current_temperature;
  // Целевая температура.
  int8_t target_temperature;
  // Производительность бризера в m3.
  uint8_t productivity;

  // Текущая потребляемая мощность тэна.
  uint8_t heater_var;

  // Время наработки бризера в секундах.
  uint32_t work_time;
  // Время наработки вентилятора в секундах.
  uint32_t fan_time;
  // Остаточный ресурс фильтра в секундах.
  uint32_t filter_time_left;
  // Счетчик воздуха прошедшего через бризер.
  uint32_t airflow_counter;
  // Счетчик воздуха прошедшего через бризер m3.
  float airflow_m3;

  uint32_t work_time_days() const { return this->work_time / (24 * 3600); }
  uint32_t filter_time_left_d() const { return this->filter_time_left / (24 * 3600); }

  // Оставшееся время работы режима "турбо" в секундах или 0 если он выключен.
  uint16_t boost_time_left;

  uint16_t firmware_version;
  uint16_t hardware_version;

  // lt/4s only sensor: ctrl pcb temperature.
  int8_t pcb_ctl_temperature;
  // 4s only sensor: pwr pcb temperature.
  int8_t pcb_pwr_temperature;

  //////////// 4S/LT
  // 4s: EC01 - ошибка заслонки
  // o2: EC15 - ошибка заслонки
  uint32_t errors;

  bool get_gate_state() const { return this->gate_position == TionGatePosition::OPENED; }

  // Текущая потребляемая мощность в Вт.
  float get_heater_power(const TionTraits &traits) const;
  // Потребляет ли сейчас обогреватель энергию.
  bool is_heating(const TionTraits &traits) const;

  // backward compatibility methods
  bool is_initialized(const TionTraits &traits) const {
    // FIXME возможно достаточно проверять fan_speed
    return traits.initialized;
  }
  const char *get_gate_position_str(const TionTraits &traits) const;
  void dump(const char *tag, const TionTraits &traits) const;
};

class TionApiBase;
class TionStateCall {
 public:
  TionStateCall(TionApiBase *api) : api_(api) {}

  void set_fan_speed(uint8_t fan_speed) { this->fan_speed_ = fan_speed; }
  void set_target_temperature(int8_t target_temperature) { this->target_temperature_ = target_temperature; }
  void set_power_state(bool power_state) { this->power_state_ = power_state; }
  void set_heater_state(bool heater_state) { this->heater_state_ = heater_state; }
  void set_led_state(bool led_state) { this->led_state_ = led_state; }
  void set_sound_state(bool sound_state) { this->sound_state_ = sound_state; }
  void set_gate_position(TionGatePosition gate_position) { this->gate_position_ = gate_position; }
  void set_auto_state(bool auto_state) { this->auto_state_ = auto_state; }

  const optional<uint8_t> &get_fan_speed() const { return this->fan_speed_; }
  const optional<bool> &get_power_state() const { return this->power_state_; }
  const optional<bool> &get_heater_state() const { return this->heater_state_; }
  const optional<int8_t> &get_target_temperature() const { return this->target_temperature_; }
  const optional<bool> &get_sound_state() const { return this->sound_state_; }
  const optional<bool> &get_led_state() const { return this->led_state_; }
  const optional<TionGatePosition> &get_gate_position() const { return this->gate_position_; }
  const optional<bool> &get_auto_state() const { return this->auto_state_; }

  // additional helper.
  void set_gate_state(bool gate_state) {
    this->gate_position_ = gate_state ? TionGatePosition::OPENED : TionGatePosition::CLOSED;
  }

  virtual void perform();

  void reset();

  void dump() const;

 protected:
  TionApiBase *api_;
  optional<uint8_t> fan_speed_;
  optional<bool> power_state_;
  optional<bool> heater_state_;
  optional<int8_t> target_temperature_;
  optional<bool> sound_state_;
  optional<bool> led_state_;
  optional<TionGatePosition> gate_position_;
  optional<bool> auto_state_;
};

std::string decode_errors(uint32_t errors, uint8_t error_min_bit, uint8_t error_max_bit, uint8_t warning_min_bit,
                          uint8_t warning_max_bit);

class TionApiBaseWriter {
 public:
  using writer_type = etl::delegate<bool(uint16_t type, const void *data, size_t size)>;
  void set_writer(writer_type &&writer) { this->writer_ = writer; }

  // Write any frame data.
  bool write_frame(uint16_t type, const void *data, size_t size) const;
  /// Write a frame with empty data.
  bool write_frame(uint16_t type) const { return this->write_frame(type, nullptr, 0); }
  /// Write a frame with empty data and request_id (4S and Lite only).
  bool write_frame(uint16_t type, uint32_t request_id) const {
    return this->write_frame(type, &request_id, sizeof(request_id));
  }
  /// Write a frame data struct.
  template<class T, std::enable_if_t<std::is_class_v<T>, bool> = true>
  bool write_frame(uint16_t type, const T &data) const {
    return this->write_frame(type, &data, sizeof(data));
  }
  /// Write a frame data struct with request id (4S and Lite only).
  template<class T, std::enable_if_t<std::is_class_v<T>, bool> = true>
  bool write_frame(uint16_t type, const T &data, uint32_t request_id) const {
    struct {
      uint32_t request_id;
      T data;
    } __attribute__((__packed__)) req{.request_id = request_id, .data = data};
    return this->write_frame(type, &req, sizeof(req));
  }

 protected:
  writer_type writer_{};
};

class TionApiBase : public TionApiBaseWriter {
  friend class TionStateCall;

  /// Callback listener for response to request_dev_info command request.
  using on_dev_info_type = etl::delegate<void(const tion_dev_info_t &dev_info)>;
  /// Callback listener for response to request_state command request.
  using on_state_type = etl::delegate<void(const TionState &state, uint32_t request_id)>;
  /// Callback listener for response to send_heartbeat command request.
  using on_heartbeat_type = etl::delegate<void(tion_dev_info_t::work_mode_t work_mode)>;

 public:
  TionApiBase();

  struct PresetData {
    // =0 - без изменений
    int8_t target_temperature;
    // <0 - без изменений, =0 - выкл, >0 - вкл
    int8_t heater_state;
    // <0 - без изменений, =0 - выкл, >0 - вкл
    int8_t power_state;
    // =0 - без изменений, >0 - вкл
    uint8_t fan_speed;
    // =UNKNOWN - без изменений
    TionGatePosition gate_position;
  };

  using on_ready_type = etl::delegate<void()>;
  /// Set callback listener for monitoring ready state
  void set_on_ready(on_ready_type &&on_ready) { this->on_ready_fn = on_ready; }

  on_ready_type on_ready_fn{};
  on_state_type on_state_fn{};
  on_dev_info_type on_dev_info_fn{};
#ifdef TION_ENABLE_HEARTBEAT
  on_heartbeat_type on_heartbeat_fn{};
#endif

  // Returns last received state.
  const TionState &get_state() const { return this->state_; }
  const TionTraits &traits() const { return this->traits_; }
  TionTraits &get_traits_() { return this->traits_; }

  virtual void request_state() = 0;
  virtual void write_state(TionStateCall *call) = 0;
  virtual void reset_filter() = 0;
  void enable_boost(bool state, TionStateCall *ext_call = nullptr);
  void set_boost_time(uint16_t boost_time);

  void enable_preset(const std::string &preset, TionStateCall *ext_call = nullptr);
  std::vector<std::string> get_presets() const;
  void add_preset(const std::string &name, const PresetData &data);
  const std::string &get_active_preset() const { return this->active_preset_; }

 protected:
  TionTraits traits_{};
  TionState state_{};
  uint32_t request_id_{};

  TionState make_write_state_(TionStateCall *call) const;

  struct : public PresetData {
    uint32_t start_time;
  } boost_save_{};

  std::map<std::string, PresetData> presets_;
  std::string active_preset_{"none"};

  virtual void enable_native_boost_(bool state) {}

  void boost_enable_(TionStateCall *ext_call);
  void boost_cancel_(TionStateCall *ext_call);
  void boost_save_state_(bool save_fan);
  void notify_state_(uint32_t request_id);
  bool chack_antifrize_(const TionState &cs) const;
  void enable_preset_(const PresetData &preset, TionStateCall *ext_call);
};

// NOLINTNEXTLINE(readability-identifier-naming)
template<class data_type> struct tion_frame_t {
  uint16_t type;
  data_type data;
  constexpr static size_t head_size() { return sizeof(type); }
} __attribute__((__packed__));
using tion_any_frame_t = tion_frame_t<uint8_t[0]>;

// NOLINTNEXTLINE(readability-identifier-naming)
template<class data_type> struct tion_ble_frame_t {
  uint16_t type;
  uint32_t ble_request_id;  // always 1
  data_type data;
  constexpr static size_t head_size() { return sizeof(type) + sizeof(ble_request_id); }
} __attribute__((__packed__));

using tion_any_ble_frame_t = tion_ble_frame_t<uint8_t[0]>;

template<class frame_spec_t> class TionProtocol {
 public:
  using frame_spec_type = frame_spec_t;

  using reader_type = etl::delegate<void(const frame_spec_t &data, size_t size)>;
  // TODO move to protected
  reader_type reader{};
  void set_reader(reader_type &&reader) { this->reader = reader; }

  using writer_type = etl::delegate<bool(const uint8_t *data, size_t size)>;
  // TODO move to protected
  writer_type writer{};
  void set_writer(writer_type &&writer) { this->writer = writer; }
};

}  // namespace tion
}  // namespace dentra
