#include <cinttypes>
#include <ctime>

#include "esphome/core/log.h"
#include "tion_component.h"

namespace esphome {
namespace tion {

static const char *const TAG = "tion_api_component";
static const char *const STATE_TIMEOUT = "state_timeout";

void TionApiComponent::call_setup() {
  PollingComponent::call_setup();
  if (this->state_timeout_ >= this->get_update_interval()) {
    ESP_LOGW(TAG, "Invalid state timout: %.1f s", this->state_timeout_ / 1000.0f);
    this->state_timeout_ = 0;
  }
  if (this->state_timeout_ == 0) {
    this->has_state_ = true;
  }
}

void TionApiComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "%s:", this->get_component_source());
  ESP_LOGCONFIG(TAG, "  Update interval: %.1f s", this->get_update_interval() / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Force update: %s", ONOFF(this->force_update_));
  ESP_LOGCONFIG(TAG, "  State timeout: %.1f s", this->state_timeout_ / 1000.0f);
  ESP_LOGCONFIG(TAG, "  Batch timeout: %.1f s", this->batch_timeout_ / 1000.0f);
  if (this->traits().supports_antifrize) {
    ESP_LOGCONFIG(TAG, "  Antifrize: enabled");
  }
}

void TionApiComponent::update() {
  this->api_->request_state();
  this->state_check_schedule_();
}

void TionApiComponent::on_state_(const TionState &state, const uint32_t request_id) {
  this->state_check_cancel_();
  this->defer([this]() {
    const auto &state = this->state();
    this->state_callback_.call(&state);
  });
  // this->state_callback_.call(&state);
}

void TionApiComponent::state_check_schedule_() {
  if (this->state_timeout_ > 0) {
    this->set_timeout(STATE_TIMEOUT, this->state_timeout_, [this]() {
      this->has_state_ = false;
      this->state_check_report_(this->state_timeout_);
    });
  } else if (!this->has_state_) {
    this->state_check_report_(this->get_update_interval());
  } else {
    this->has_state_ = false;
  }
}

void TionApiComponent::state_check_report_(uint32_t timeout) {
  ESP_LOGW(TAG, "State was not received in %.1fs", timeout / 1000.0f);
  this->state_callback_.call(nullptr);
}

void TionApiComponent::state_check_cancel_() {
  this->has_state_ = true;
  if (this->state_timeout_ > 0) {
    this->cancel_timeout(STATE_TIMEOUT);
  }
}

dentra::tion::TionStateCall *TionApiComponent::make_call() {
  if (this->batch_call_) {
    ESP_LOGD(TAG, "Continue batch update for %u ms", this->batch_timeout_);
  } else {
    ESP_LOGD(TAG, "Starting batch update for %u ms", this->batch_timeout_);
    this->batch_call_ = new BatchStateCall(this->api_, this, this->batch_timeout_, [this]() { this->batch_write_(); });
  }
  return this->batch_call_;
}

void TionApiComponent::batch_write_() {
  BatchStateCall *batch_call = this->batch_call_;
  this->batch_call_ = nullptr;
  delete batch_call;
  this->state_check_schedule_();
}

#ifdef TION_ENABLE_SCHEDULER

void Tion4sApiComponent::on_time(time_t time, uint32_t request_id) {
  auto *c_tm = std::gmtime(&time);
  char buf[20] = {};
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGI(TAG, "Device UTC time: %s", buf);
  c_tm = std::localtime(&time);
  std::strftime(buf, sizeof(buf), "%F %T", c_tm);
  ESP_LOGI(TAG, "Device local time: %s", buf);
}

void Tion4sApiComponent::on_timer(uint8_t timer_id, const dentra::tion_4s::tion4s_timer_t &timer, uint32_t request_id) {
  std::string schedule;
  if (timer.schedule.monday && timer.schedule.tuesday && timer.schedule.wednesday && timer.schedule.thursday &&
      timer.schedule.friday && timer.schedule.saturday && timer.schedule.sunday) {
    schedule = "MON-SUN";
  } else {
    auto add_timer_week_day = [](std::string &s, bool day, const char *day_name) {
      if (day) {
        if (!s.empty()) {
          s.append(", ");
        }
        s.append(day_name);
      }
    };
    add_timer_week_day(schedule, timer.schedule.monday, "MON");
    add_timer_week_day(schedule, timer.schedule.tuesday, "TUE");
    add_timer_week_day(schedule, timer.schedule.wednesday, "WED");
    add_timer_week_day(schedule, timer.schedule.thursday, "THU");
    add_timer_week_day(schedule, timer.schedule.friday, "FRI");
    add_timer_week_day(schedule, timer.schedule.saturday, "SAT");
    add_timer_week_day(schedule, timer.schedule.sunday, "SUN");
  }

  ESP_LOGI(TAG, "Timer[%u] %s at %02u:%02u is %s", timer_id, schedule.c_str(), timer.schedule.hours,
           timer.schedule.minutes, ONOFF(timer.timer_state));
}

void Tion4sApiComponent::on_timers_state(const dentra::tion_4s::tion4s_timers_state_t &timers_state,
                                         uint32_t request_id) {
  for (int i = 0; i < tion4s_timers_state_t::TIMERS_COUNT; i++) {
    ESP_LOGI(TAG, "Timer[%d] state %s", i, ONOFF(timers_state.timers[i].active));
  }
}

void Tion4sApiComponent::dump_timers() {
  this->api_->request_time(++this->request_id_);
  for (uint8_t timer_id = 0; timer_id < dentra::tion_4s::tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    this->api_->request_timer(timer_id, ++this->request_id_);
  }
  this->api_->request_timers_state(++this->request_id_);
}

void Tion4sApiComponent::reset_timers() {
  const dentra::tion_4s::tion4s_timer_t timer{};
  for (uint8_t timer_id = 0; timer_id < dentra::tion_4s::tion4s_timers_state_t::TIMERS_COUNT; timer_id++) {
    this->api_->write_timer(timer_id, timer, ++this->request_id_);
  }
}
#endif  // TION_ENABLE_SCHEDULER

}  // namespace tion
}  // namespace esphome
