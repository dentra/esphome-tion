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

FLAGS="-rdynamic -fno-rtti"

if [ "$1" != "debug" ]; then
  FLAGS="-O3 $FLAGS"
else
  FLAGS="-g3 $FLAGS"
fi

OUT=.esphome/main.out

g++ $FLAGS ${SRCS[@]/#/-g } ${DEFS[@]/#/-D} -o $OUT ${LIBS[@]/#/-l}

if [ $? -eq 0 -a "$1" != "debug" ]; then
  $OUT
fi

