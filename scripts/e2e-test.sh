#!/bin/bash

# E2E tests: host firmware from tests/smoke with breezer emulators on pty.
# example usage: e2e-test.sh [pytest options], e.g. e2e-test.sh -k boost
# set TION_E2E_NO_BUILD=1 to skip firmware rebuild
# firmware logs: build.tmp/logs/e2e/<test>.log

prj_path=$(realpath $(dirname $0)/..)

if [[ -z "${VIRTUAL_ENV}" && -f $prj_path/.venv/bin/activate ]]; then
  source $prj_path/.venv/bin/activate
fi

cd $prj_path && python -m pytest "$@"
