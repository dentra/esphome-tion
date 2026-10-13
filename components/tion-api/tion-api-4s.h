#pragma once

#include <functional>

#include "tion-api-writer.h"
#include "tion-api-4s-internal.h"

namespace dentra {
namespace tion_4s {

class Tion4sApi : public tion::TionApiBase, public tion::TionApiWriter {
  /// Callback listener for response to request_turbo command request.
  using on_turbo_type = std::function<void(const tion4s_turbo_t &turbo, uint32_t request_id)>;
#ifdef TION_ENABLE_SCHEDULER
  /// Callback listener for response to request_time command request.
  using on_time_type = std::function<void(time_t time, uint32_t request_id)>;
  /// Callback listener for response to request_timer command request.
  using on_timer_type = std::function<void(uint8_t timer_id, const tion4s_timer_t &timers_state, uint32_t request_id)>;
  /// Callback listener for response to request_timers_state command request.
  using on_timers_state_type = std::function<void(const tion4s_timers_state_t &timers_state, uint32_t request_id)>;
#endif

 public:
  Tion4sApi();

  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool write_state(const tion::TionState &state, uint32_t request_id) const;
  bool reset_filter(const tion::TionState &state, uint32_t request_id = 1) const;
  bool factory_reset(const tion::TionState &state, uint32_t request_id = 1) const;
  bool reset_errors(const tion::TionState &state, uint32_t request_id = 1) const;

  /// Callback listener for response to request_turbo command request.
  void set_on_turbo(on_turbo_type &&cb) { this->on_turbo_ = std::move(cb); }
  bool set_turbo(uint16_t time, uint32_t request_id = 1) const;
  bool request_turbo() const { return this->request_turbo_(); }

#ifdef TION_ENABLE_HEARTBEAT
  bool send_heartbeat() const;
#endif

#ifdef TION_ENABLE_SCHEDULER
  bool request_time(uint32_t request_id) const;
  void request_time() { this->request_time(++this->request_id_); }

  /// Callback listener for response to request_time command request.
  void set_on_time(on_time_type &&cb) { this->on_time_ = std::move(cb); }
  bool set_time(time_t time, uint32_t request_id) const;

  /// Callback listener for response to request_timer command request.
  void set_on_timer(on_timer_type &&cb) { this->on_timer_ = std::move(cb); }
  bool request_timer(uint8_t timer_id, uint32_t request_id) const;
  void request_timer(uint8_t timer_id) { this->request_timer(timer_id, ++this->request_id_); }

  /// Request all timers.
  bool request_timers(uint32_t request_id = 1) const;

  bool write_timer(uint8_t timer_id, const tion4s_timer_t &timer, uint32_t request_id) const;
  void write_timer(uint8_t timer_id, const tion4s_timer_t &timer) {
    this->write_timer(timer_id, timer, ++this->request_id_);
  }

  bool request_timers_state(uint32_t request_id) const;
  void request_timers_state() { this->request_timers_state(++this->request_id_); }
  /// Callback listener for response to request_timers_state command request.
  void set_on_timers_state(on_timers_state_type &&cb) { this->on_timers_state_ = std::move(cb); }

#endif
#ifdef TION_ENABLE_DIAGNOSTIC
  bool request_errors() const;
  bool request_test() const;
#endif

  void enable_native_boost_support();
  void request_state() override;
  void write_state(tion::TionStateCall *call) override {
    this->write_state(this->make_write_state_(call), ++this->request_id_);
  }
  void reset_filter() override { this->reset_filter(this->state_, ++this->request_id_); }

  uint16_t get_boost_time_left() const override;
  bool is_boost_running() const override;

 protected:
  on_turbo_type on_turbo_{};
#ifdef TION_ENABLE_SCHEDULER
  on_time_type on_time_{};
  on_timer_type on_timer_{};
  on_timers_state_type on_timers_state_{};
#endif
  uint16_t native_boost_time_left_{};

  void enable_boost(uint16_t boost_time, tion::TionStateCall *call) override;

  bool request_turbo_() const;
  bool request_dev_info_() const;
  bool request_state_() const;

  void dump_state_(const tion4s_state_t &state) const;
  void update_state_(const tion4s_state_t &state);
  void update_dev_info_(const tion::tion_dev_info_t &dev_info);
  void update_turbo_(const tion4s_turbo_t &turbo);
};

}  // namespace tion_4s
}  // namespace dentra
