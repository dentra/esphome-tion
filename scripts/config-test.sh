#!/bin/bash

# Configuration tests: validates tests/gen configs with `esphome config` and checks the result.
# example usage: config-test.sh [pytest options]

prj_path=$(realpath $(dirname $0)/..)

if [[ -z "${VIRTUAL_ENV}" && -f $prj_path/.venv/bin/activate ]]; then
  source $prj_path/.venv/bin/activate
fi

cd $prj_path && python -m pytest tests/config "$@"
