cmake_minimum_required(VERSION 3.18)
project(tion_unit_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TION_SANITIZE "Build with address and undefined behavior sanitizers" ON)
if(TION_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

set(TION_API_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../components/tion-api")

file(GLOB tion_api_SRC CONFIGURE_DEPENDS "${TION_API_DIR}/*.cpp")
add_library(tion_api STATIC ${tion_api_SRC})
target_include_directories(tion_api PUBLIC "${TION_API_DIR}")
# all optional features enabled, TION_ESPHOME is not defined
target_compile_definitions(tion_api PUBLIC
  TION_LOG_LEVEL=6
  TION_ENABLE_HEARTBEAT
  TION_ENABLE_SCHEDULER
  TION_ENABLE_DIAGNOSTIC
  TION_ENABLE_PI_CONTROLLER
  TION_ENABLE_ANTIFREEZE
  TION_ENABLE_KIV
)

file(GLOB unit_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_executable(unit-tests ${unit_SRC})
target_include_directories(unit-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(unit-tests PRIVATE tion_api)
