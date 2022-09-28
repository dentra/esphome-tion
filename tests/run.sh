#!/bin/bash

SRCS=(
    tests/*.cpp
    components/tion-api/*.cpp
)

DEFS=(
    TION_LOG_LEVEL=6
)

LIBS=(
    mbedtls
    mbedcrypto
)
FLAGS=""
FLAGS="$FLAGS -rdynamic" # exports the symbols of an executable, used for obtain a backtrace and print it to stdout
FLAGS="$FLAGS -fno-rtti -fno-exceptions"
FLAGS="$FLAGS -DTION_ENABLE_TESTS -DTION_ENABLE_HEARTBEAT -DTION_ENABLE_PRESETS -DTION_ENABLE_SCHEDULER -DTION_ENABLE_DIAGNOSTIC"

if [ "$1" != "debug" ]; then
  FLAGS="-O3 $FLAGS"
else
  FLAGS="-g3 $FLAGS"
fi

OUT=.esphome/main.out

g++ $FLAGS ${SRCS[@]/#/-g } ${DEFS[@]/#/-D} -o $OUT ${LIBS[@]/#/-l} -I ".esphome/libdeps/esp32-arduino/Embedded Template Library/include"

if [ $? -eq 0 -a "$1" != "debug" -a "$1" != "info" ]; then
size $OUT
$OUT
fi

