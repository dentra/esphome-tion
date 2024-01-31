#!/bin/bash

TGT=all
TYP=Debug

if [ "$1" == "clean" ]; then
  TGT=clean
fi

ARGS="--parallel"
if [ "$1" == "clean-first" ]; then
  ARGS+=" --clean-first"
fi
if [ "$1" == "verbose" ]; then
  ARGS="--verbose"
fi

if [ -z "$PLATFORMIO_CORE_DIR" ]; then
  export PLATFORMIO_CORE_DIR=$(pio system info --json-output|jq -r .core_dir.value)
fi

if [[ -z "${BUILD_DIR}" ]]; then
  export BUILD_DIR=".build"
fi

EX_TEST_DEFINES=$(IFS=$';'; echo "${DEFS[*]}")
EX_TEST_SOURCES=$(IFS=$';'; echo "${SRCS[*]}")
EX_TEST_INCLUDES=$(IFS=$';'; echo "${INCS[*]}")
EX_TEST_SOURCES_ESPHOME=$(IFS=$';'; echo "${SRCS_ESPHOME[*]}")

echo EX_TEST_DEFINES=$EX_TEST_DEFINES
echo EX_TEST_SOURCES=$EX_TEST_SOURCES
echo EX_TEST_INCLUDES=$EX_TEST_INCLUDES
echo EX_TEST_SOURCES_ESPHOME=$EX_TEST_SOURCES_ESPHOME

BLD="$BUILD_DIR/tests"
cmake -B $BLD -S $(dirname $0) -DCMAKE_BUILD_TYPE=$TYP \
  -DEX_TEST_DEFINES="$EX_TEST_DEFINES" \
  -DEX_TEST_INCLUDES="$EX_TEST_INCLUDES" \
  -DEX_TEST_SOURCES="$EX_TEST_SOURCES" \
  -DEX_TEST_FILTER="$SRCS_FILTER" \
  -DEX_TEST_SOURCES_ESPHOME="$EX_TEST_SOURCES_ESPHOME" && \
cmake --build $BLD --target $TGT --config $TYP $ARGS || exit $?

if [ $TGT == "clean" ]; then
  exit 0
fi

OUT=$BLD/tests
if [ "$1" == "info" ]; then
  size $OUT
elif [ "$1" != "build" ]; then
  $OUT $*
fi
