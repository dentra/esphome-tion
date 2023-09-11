#pragma once
#include "esphome/core/defines.h"

#ifdef USE_FAN
#include "../tion-api/tion-api.h"

#include "tion_component.h"
#include "tion_fan.h"
#include "tion_vport.h"

namespace esphome {
namespace tion {

/**
 * @param tion_api_type TionApi implementation.
 * @param tion_state_type Tion state struct.
 */
template<class tion_api_type> class TionFanComponent {};

}  // namespace tion
}  // namespace esphome
#endif
