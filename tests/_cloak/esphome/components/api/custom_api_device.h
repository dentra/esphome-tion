#pragma once

#include <map>

namespace esphome {
namespace api {

// template<typename T, typename... Ts> class CustomAPIDeviceService : public UserServiceBase<Ts...> {
//  public:
//   CustomAPIDeviceService(const std::string &name, const std::array<std::string, sizeof...(Ts)> &arg_names, T *obj,
//                          void (T::*callback)(Ts...))
//       : UserServiceBase<Ts...>(name, arg_names), obj_(obj), callback_(callback) {}

//  protected:
//   void execute(Ts... x) override { (this->obj_->*this->callback_)(x...); }  // NOLINT

//   T *obj_;
//   void (T::*callback_)(Ts...);
// };

class CustomAPIDevice {
 public:
  /// Return if a client (such as Home Assistant) is connected to the native API.
  bool is_connected() const { return false; }

  /** Register a custom native API service that will show up in Home Assistant.
   *
   * Usage:
   *
   * ```cpp
   * void setup() override {
   *   register_service(&CustomNativeAPI::on_start_washer_cycle, "start_washer_cycle",
   *                    {"cycle_length"});
   * }
   *
   * void on_start_washer_cycle(int cycle_length) {
   *   // Start washer cycle.
   * }
   * ```
   *
   * @tparam T The class type creating the service, automatically deduced from the function pointer.
   * @tparam Ts The argument types for the service, automatically deduced from the function arguments.
   * @param callback The member function to call when the service is triggered.
   * @param name The name of the service to register.
   * @param arg_names The name of the arguments for the service, must match the arguments of the function.
   */
  template<typename T, typename... Ts>
  void register_service(void (T::*callback)(Ts...), const std::string &name,
                        const std::array<std::string, sizeof...(Ts)> &arg_names) {}

  /** Register a custom native API service that will show up in Home Assistant.
   *
   * Usage:
   *
   * ```cpp
   * void setup() override {
   *   register_service(&CustomNativeAPI::on_hello_world, "hello_world");
   * }
   *
   * void on_hello_world() {
   *   // Hello World service called.
   * }
   * ```
   *
   * @tparam T The class type creating the service, automatically deduced from the function pointer.
   * @param callback The member function to call when the service is triggered.
   * @param name The name of the arguments for the service, must match the arguments of the function.
   */
  template<typename T> void register_service(void (T::*callback)(), const std::string &name) {}

  /** Subscribe to the state (or attribute state) of an entity from Home Assistant.
   *
   * Usage:
   *
   * ```cpp
   * void setup() override {
   *   subscribe_homeassistant_state(&CustomNativeAPI::on_state_changed, "climate.kitchen", "current_temperature");
   * }
   *
   * void on_state_changed(std::string state) {
   *   // State of sensor.weather_forecast is `state`
   * }
   * ```
   *
   * @tparam T The class type creating the service, automatically deduced from the function pointer.
   * @param callback The member function to call when the entity state changes.
   * @param entity_id The entity_id to track.
   * @param attribute The entity state attribute to track.
   */
  template<typename T>
  void subscribe_homeassistant_state(void (T::*callback)(std::string), const std::string &entity_id,
                                     const std::string &attribute = "") {}

  /** Subscribe to the state (or attribute state) of an entity from Home Assistant.
   *
   * Usage:
   *å
   * ```cpp
   * void setup() override {
   *   subscribe_homeassistant_state(&CustomNativeAPI::on_state_changed, "sensor.weather_forecast");
   * }
   *
   * void on_state_changed(std::string entity_id, std::string state) {
   *   // State of `entity_id` is `state`
   * }
   * ```
   *
   * @tparam T The class type creating the service, automatically deduced from the function pointer.
   * @param callback The member function to call when the entity state changes.
   * @param entity_id The entity_id to track.
   * @param attribute The entity state attribute to track.
   */
  template<typename T>
  void subscribe_homeassistant_state(void (T::*callback)(std::string, std::string), const std::string &entity_id,
                                     const std::string &attribute = "") {}

  /** Call a Home Assistant service from ESPHome.
   *
   * Usage:
   *
   * ```cpp
   * call_homeassistant_service("homeassistant.restart");
   * ```
   *
   * @param service_name The service to call.
   */
  void call_homeassistant_service(const std::string &service_name) {}

  /** Call a Home Assistant service from ESPHome.
   *
   * Usage:
   *
   * ```cpp
   * call_homeassistant_service("light.turn_on", {
   *   {"entity_id", "light.my_light"},
   *   {"brightness", "127"},
   * });
   * ```
   *
   * @param service_name The service to call.
   * @param data The data for the service call, mapping from string to string.
   */
  void call_homeassistant_service(const std::string &service_name, const std::map<std::string, std::string> &data) {}

  /** Fire an ESPHome event in Home Assistant.
   *
   * Usage:
   *
   * ```cpp
   * fire_homeassistant_event("esphome.something_happened");
   * ```
   *
   * @param event_name The event to fire.
   */
  void fire_homeassistant_event(const std::string &event_name) {}

  /** Fire an ESPHome event in Home Assistant.
   *
   * Usage:
   *
   * ```cpp
   * fire_homeassistant_event("esphome.something_happened", {
   *   {"my_value", "500"},
   * });
   * ```
   *
   * @param event_name The event to fire.
   * @param data The data for the event, mapping from string to string.
   */
  void fire_homeassistant_event(const std::string &service_name, const std::map<std::string, std::string> &data) {}
};

}  // namespace api
}  // namespace esphome
