#!/bin/bash

if [ $USER != "vscode" ]; then
  WORKSPACE_DIR=$(realpath $(dirname $BASH_SOURCE)/..)
fi

ESPHOME_LIB_DIR="$WORKSPACE_DIR/.esphome/include"
ARDUINO_LIB_DIR="$WORKSPACE_DIR/.esphome/libdeps/esp32-arduino"

DEFS=(
  TION_ESPHOME
  TION_LOG_LEVEL=6
  TION_ENABLE_TESTS
  TION_ENABLE_HEARTBEAT
  TION_ENABLE_PRESETS
  TION_ENABLE_SCHEDULER
  TION_ENABLE_DIAGNOSTIC
  USE_VPORT_UART
  USE_VPORT_BLE
)

VPORT_DIR=$WORKSPACE_DIR/components/vport
if [ ! -e $VPORT_DIR ]; then
  VPORT_DIR=""
fi

if [ -z "$VPORT_DIR" ]; then
 SRCS=(
    "$ESPHOME_LIB_DIR/esphome-components/esphome/components/vport/*.cpp"
  )
else
  SRCS=(
    "$VPORT_DIR/*.cpp"
  )
  SRCS_ESPHOME=(
    $VPORT_DIR
  )
fi

SRCS_FILTER=".*/(esp32_usb_dis|logger|wifi)/.+\\.cpp$"

INCS=(
  "$ESPHOME_LIB_DIR/esphome-components"
  "$ARDUINO_LIB_DIR/Embedded Template Library/include"
)

.  $(dirname $0)/_cloak/runner.sh
