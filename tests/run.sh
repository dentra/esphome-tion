#!/bin/bash

if [ $USER != "vscode" ]; then
  WORKSPACE_DIR=$(realpath $(dirname $BASH_SOURCE)/..)
fi

if [[ -z "${VIRTUAL_ENV}" ]]; then
  echo "activate venv"
  source $WORKSPACE_DIR/.venv/bin/activate
fi

export BUILD_DIR="$WORKSPACE_DIR/.build"

ESPHOME_LIB_DIR="$BUILD_DIR/esphome/include"

DEFS=(
  TION_ESPHOME
  TION_LOG_LEVEL=6
  TION_ENABLE_TESTS
  TION_ENABLE_HEARTBEAT
  TION_ENABLE_SCHEDULER
  TION_ENABLE_DIAGNOSTIC
  USE_VPORT_UART
  USE_VPORT_BLE
  USE_VPORT_COMMAND_QUEUE_SIZE=16
)

LIB_DIR=$WORKSPACE_DIR/lib
LIB_ETL_DIR=$LIB_DIR/etl
LIB_COMPO_DIR=$LIB_DIR/esphome-components
LIB_COMPO_VPORT_DIR="$LIB_COMPO_DIR/esphome/components/vport"

SRCS=(
  "$LIB_COMPO_VPORT_DIR/*.cpp"
  "$LIB_COMPO_DIR/esphome/components/nvs/*.cpp"
)

SRCS_ESPHOME=(
  # $LIB_COMPO_VPORT_DIR
)

SRCS_FILTER=".*/(esp32_usb_dis|logger|wifi|coredump|settings|tion_3s_rc|_?web_server|web_[_a-z]+|esp32_[_a-z]+)/.+\\.cpp$"


INCS=(
  "$LIB_COMPO_DIR"
  "$LIB_ETL_DIR/include"
)

.  $(dirname $0)/_cloak/runner.sh
