#pragma once

#include <functional>

#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"

#include "../tion-api/tion-api.h"

namespace esphome {
namespace tion {

class BatchStateCall : public dentra::tion::TionStateCall {
 public:
  explicit BatchStateCall(dentra::tion::TionApiBase *api, Component *component, uint32_t timeout,
                          std::function<void()> &&write_out)
      : dentra::tion::TionStateCall(api), c_(component), timeout_(timeout), write_out_(std::move(write_out)) {}

  virtual ~BatchStateCall() {}

  void perform() override;

 protected:
  Component *c_;
  uint32_t timeout_;
  std::function<void()> write_out_;
  void perform_();
};

}  // namespace tion
}  // namespace esphome
