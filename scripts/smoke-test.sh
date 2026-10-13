#!/bin/bash

# Fast smoke test: compiles all breezer types with all entities and auto_co2 package on host platform.
# example usage: smoke-test.sh [-v]

prj_path=$(realpath $(dirname $0)/..)

if [[ -z "${VIRTUAL_ENV}" && -f $prj_path/.venv/bin/activate ]]; then
  source $prj_path/.venv/bin/activate
fi

# do not let esphome create .gitignore next to configs
export ESPHOME_NOGITIGNORE=1
export ESPHOME_DATA_DIR=$prj_path/build.tmp/esphome

# use local esphome-components when linked into components dir
subs=()
if [ -e $prj_path/components/vport ]; then
  subs=(-s vport_source $prj_path/components)
fi

esphome $1 "${subs[@]}" compile $prj_path/tests/smoke/tion-smoke.yaml || exit $?
# auto_co2 package needs tion as a mapping, see tion-auto-co2.yaml
esphome $1 "${subs[@]}" compile $prj_path/tests/smoke/tion-auto-co2.yaml
