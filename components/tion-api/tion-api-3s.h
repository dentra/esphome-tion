#pragma once

#include <functional>

#include "tion-api.h"
#include "tion-api-3s-internal.h"

namespace dentra {
namespace tion {

class Tion3sApi : public TionApiBase {
 public:
  Tion3sApi();

  void read_frame(uint16_t frame_type, const void *frame_data, size_t frame_data_size);

  uint16_t get_state_type() const;

  bool pair() const;

  bool request_command4() const;
  bool write_state(const tion::TionState &state, uint32_t unused_request_id) const;
  bool reset_filter(const tion::TionState &state) const;
  bool factory_reset(const tion::TionState &state) const;

  void request_state() override;
  void write_state(tion::TionStateCall *call) override {
    this->write_state(this->make_write_state_(call), ++this->request_id_);
  }
  void reset_filter() override { this->reset_filter(this->state_); }

 protected:
  bool request_state_() const;

  void dump_state_(const tion_3s::tion3s_state_t &state) const;
  void update_state_(const tion_3s::tion3s_state_t &state);
};

}  // namespace tion
}  // namespace dentra
