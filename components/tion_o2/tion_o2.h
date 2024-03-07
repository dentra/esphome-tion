#pragma once

#include "../tion-api/tion-api-o2.h"
#include "../tion/tion_api_component.h"

namespace esphome {
namespace tion {

using TionO2Api = dentra::tion_o2::TionO2Api;

// class TionO2 : public dentra::tion_o2::TionO2Api {
//  public:
//   TionO2() : dentra::tion_o2::TionO2Api() {}
// };

class TionO2ApiComponent : public TionApiComponentBase<TionO2Api> {
 public:
  explicit TionO2ApiComponent(TionO2Api *api, TionVPortType vport_type) : TionApiComponentBase(api, vport_type) {}
};

}  // namespace tion
}  // namespace esphome
