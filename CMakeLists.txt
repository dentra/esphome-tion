cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esphome-tion)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++2a" APPEND)

add_definitions(
    -DTION_ESPHOME
    -DTION_ENABLE_HEARTBEAT
    -DTION_ENABLE_SCHEDULER
    -DTION_ENABLE_DIAGNOSTIC
    -DTION_ENABLE_ANTIFREEZE
    -DUSE_VPORT_BLE
    -DUSE_VPORT_UART
    -DUSE_VPORT_JTAG
    -DUSE_QINGPING_EXPLORER

    -DESPHOME_LOG_LEVEL=ESPHOME_LOG_LEVEL_VERY_VERBOSE
    -DUSE_ESP32
    -DUSE_ESP_IDF
    -DUSE_ESP32_FRAMEWORK_ESP_IDF
    -DUSE_ESP32_VARIANT_ESP32
    -DAUDIO_NO_SD_FS
    -DHAVE_CONFIG_H
)

FILE(GLOB_RECURSE app_sources
    ${CMAKE_SOURCE_DIR}/components/**/*.cpp
    ${CMAKE_SOURCE_DIR}/components/**/*.h
)
target_sources(esphome-tion.elf PRIVATE ${app_sources})
# idf_component_register(SRCS ${app_sources})

set(WORKSPACE_DIR "${CMAKE_SOURCE_DIR}")
set(ESPHOME_DATA_DIR "${WORKSPACE_DIR}/.build/esphome")

include_directories("${WORKSPACE_DIR}") # will include esphome
include_directories("${WORKSPACE_DIR}/lib/esphome-components")
include_directories("${WORKSPACE_DIR}/lib/components-alias")
include_directories("${WORKSPACE_DIR}/lib/etl/include")
# include_directories("${WORKSPACE_DIR}/lib/gsl-lite/include")
# include_directories(SYSTEM "${ESPHOME_DATA_DIR}/libdeps/esp32-arduino/ArduinoJson/src")
include_directories(SYSTEM "${WORKSPACE_DIR}/lib/ArduinoJson/src")
set("PIO_ESP32" $ENV{HOME}/.platformio/packages/framework-arduinoespressif32)
include_directories(SYSTEM "${PIO_ESP32}/tools/sdk/esp32/include/esp32-camera/driver/include")
include_directories(SYSTEM "${PIO_ESP32}/tools/sdk/esp32/include/esp32-camera/conversions/include")
