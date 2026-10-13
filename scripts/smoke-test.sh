#!/bin/bash

# Fast smoke test: compiles all breezer types with all entities on host platform.
# example usage: smoke-test.sh [-v]

prj_path=$(realpath $(dirname $0)/..)

if [[ -z "${VIRTUAL_ENV}" && -f $prj_path/.venv/bin/activate ]]; then
  source $prj_path/.venv/bin/activate
fi

export ESPHOME_DATA_DIR=$prj_path/build.tmp/esphome

# use local esphome-components when linked into components dir
subs=()
if [ -e $prj_path/components/vport ]; then
  subs=(-s vport_source $prj_path/components)
fi

esphome $1 "${subs[@]}" compile $prj_path/tests/smoke/tion-smoke.yaml
