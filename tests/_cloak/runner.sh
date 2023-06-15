#!/bin/bash

TGT=all
TYP=release

if [ "$1" == "clean" ]; then
  TGT=clean
fi

if [ "$1" == "debug" ]; then
  TYP=debug
fi

ARGS="--parallel"
if [ "$1" == "clean-first" ]; then
  ARGS+=" --clean-first"
fi
if [ "$1" == "verbose" ]; then
  ARGS="--verbose"
fi

EX_TEST_DEFINES=$(IFS=$';'; echo "${DEFS[*]}")
EX_TEST_SOURCES=$(IFS=$';'; echo "${SRCS[*]}")
EX_TEST_INCLUDES=$(IFS=$';'; echo "${INCS[*]}")

BLD=.esphome/tests/$TYP
cmake -B $BLD -S $(dirname $0) -DCMAKE_BUILD_TYPE=$TYP \
  -DEX_TEST_DEFINES="$EX_TEST_DEFINES" \
  -DEX_TEST_INCLUDES="$EX_TEST_INCLUDES" \
  -DEX_TEST_SOURCES="$EX_TEST_SOURCES" \
  -DEX_TEST_FILTER="$SRCS_FILTER" && \
cmake --build $BLD --target $TGT $ARGS || exit $?

OUT=$BLD/tests

if [ "$1" == "info" ]; then
  size $OUT
elif [ "$1" != "debug" ]; then
  $OUT $*
fi
