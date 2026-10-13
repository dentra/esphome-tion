#!/bin/bash

# Unit tests of tion-api without ESPHome.
# example usage: unit-test.sh [build | doctest options], e.g. unit-test.sh -tc="4s*"
# set TION_LOG=1 to see tion-api log output

prj_path=$(realpath $(dirname $0)/..)

if [[ -z "${VIRTUAL_ENV}" && -f $prj_path/.venv/bin/activate ]]; then
  source $prj_path/.venv/bin/activate
fi

bld=$prj_path/build.tmp/unit

cmake -S $prj_path/tests/unit -B $bld -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug > /dev/null || exit $?
cmake --build $bld --parallel || exit $?

if [ "$1" == "build" ]; then
  exit 0
fi

$bld/unit-tests "$@"
