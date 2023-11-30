cmake_minimum_required(VERSION 3.18)
project(cloak VERSION 1.0 LANGUAGES CXX)

set(ESP_SDK_DIR "$ENV{PLATFORMIO_CORE_DIR}/packages/framework-arduinoespressif32/tools/sdk/esp32")
set(ESP_SDK_INCLUDE_DIR "${ESP_SDK_DIR}/include")

# time : timezone -> timezone_t
file(GLOB cloak_SRC
  *.cpp
  esphome/core/*.cpp
  esphome/components/logger/*.cpp
  esphome/components/uart/*.cpp
  esphome/components/sensor/*.cpp
  esphome/components/switch/*.cpp
  esphome/components/ble_client/*.cpp
  esphome/components/esp32_ble_client/*.cpp
  esphome/components/esp32_ble_tracker/*.cpp
  esphome/components/climate/*.cpp
  esphome/components/text_sensor/*.cpp
  esphome/components/binary_sensor/*.cpp
  esphome/components/select/*.cpp
  esphome/components/number/*.cpp
  esphome/components/time/*.cpp
  esp32/*.cpp
)

set(ESP_SDK_INCLUDE
  "${ESP_SDK_INCLUDE_DIR}/log/include"
  "${ESP_SDK_INCLUDE_DIR}/esp_rom/include"
  "${ESP_SDK_INCLUDE_DIR}/esp_common/include"
  "${ESP_SDK_INCLUDE_DIR}/esp_system/include"
  "${ESP_SDK_INCLUDE_DIR}/esp_hw_support/include"
  "${ESP_SDK_INCLUDE_DIR}/freertos/include"
  "${ESP_SDK_INCLUDE_DIR}/bt/host/bluedroid/api/include/api"
  "${ESP_SDK_INCLUDE_DIR}/bt/include/esp32/include"
  "${ESP_SDK_INCLUDE_DIR}/soc/esp32/include"
)

set(ESP_SDK_INCLUDE ${ESP_SDK_INCLUDE} PARENT_SCOPE)

add_library(${PROJECT_NAME} ${cloak_SRC})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

target_compile_options(${PROJECT_NAME} PUBLIC
    # -rdynamic
    -fno-rtti
    -fno-exceptions
    -Wno-extern-c-compat
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  USE_TESTS
  USE_ESP32
  USE_ESP32_FRAMEWORK_ARDUINO
  ESPHOME_LOG_LEVEL=7
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/esp32
  ${ESP_SDK_INCLUDE}
)

