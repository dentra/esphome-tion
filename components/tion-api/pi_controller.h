#pragma once

#include <cstdint>
#include <cmath>

#include "utils.h"

namespace dentra {
namespace tion {
namespace auto_co2 {

/// @brief PI Controller.
/// @link https://www.sciencedirect.com/science/article/pii/S0378778823009477
class PIController {
 public:
  /// @param kp proportional gain [L/s.ppm-CO2]
  /// @param ti integral gain [min]
  /// @param db dead band [ppm]
  /// @param v_oa_min minimum outdoor airflow rate [L/s]
  /// @param v_oa_max maximum outdoor airflow rate [L/s]
  PIController(float kp, float ti, int db = 0, float min = NAN, float max = NAN)
      : kp_(kp), ti_(ti), db_(db), v_oa_min_(min), v_oa_max_(max) {}

  /// @param setpoint CO2 setpoint [ppm]
  /// @param current CO2 concentration [ppm]
  /// @return outdoor airflow rate [L/s]
  float update(int setpoint, int current);

  void set_min(float min) { this->v_oa_min_ = min; }
  void set_max(float max) { this->v_oa_min_ = max; }

  /// @brief Resets integral error.
  void reset() { this->ib_ = 0; }
  /// @brief Resets Kp, Ti, db without touching min and max
  void reset(float kp, float ti, int db);
  void reset(float kp, float ti, int db, float min, float max);

 protected:
  /// proportional gain [L/s.ppm-CO2]
  float kp_;
  /// integral gain [min]
  float ti_;
  /// dead band [ppm]
  int db_;
  /// minimum outdoor airflow rate [L/s]
  float v_oa_min_;
  /// maximum outdoor airflow rate [L/s]
  float v_oa_max_;
  /// integral error with anti-integral windup [min.ppm-CO2].
  float ib_{};
  /// last time used in dt_() calculation.
  uint32_t last_time_{};

  /// time step [s].
  float dt_() {
      const auto now = tion::millis();
  const float dt = this->last_time_ == 0 ? 0.0f : (now - this->last_time_) * 0.001f;
  this->last_time_ = now;
  return dt;
  }
};

}  // namespace auto_co2
}  // namespace tion
}  // namespace dentra
