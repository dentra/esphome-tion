#!/bin/bash

SRCS=(
    tests/*.cpp
    components/tion-api/*.cpp
)

LIBS=(
    mbedtls
    mbedcrypto
)

FLAGS=(
  -rdynamic # exports the symbols of an executable, used for obtain a backtrace and print it to stdout
  -fno-rtti
  -fno-exceptions
)

DEFS=(
  TION_LOG_LEVEL=6
  TION_ENABLE_TESTS
  TION_ENABLE_HEARTBEAT
  TION_ENABLE_PRESETS
  TION_ENABLE_SCHEDULER
  TION_ENABLE_DIAGNOSTIC
)

if [ "$1" != "debug" ]; then
  FLAGS="-O3 $FLAGS"
else
  FLAGS="-g3 $FLAGS"
fi

OUT=.esphome/main.out

g++ ${FLAGS[@]} ${SRCS[@]/#/-g } ${DEFS[@]/#/-D} -o $OUT ${LIBS[@]/#/-l} -I ".esphome/libdeps/esp32-arduino/Embedded Template Library/include"

if [ $? -eq 0 -a "$1" != "debug" -a "$1" != "info" ]; then
size $OUT
$OUT
fi

