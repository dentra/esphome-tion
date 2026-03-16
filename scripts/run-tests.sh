#!/bin/bash

# example usage: run-tests.sh -t lt -p uart -c api -v

prj_path=$(dirname $0)/..

types=(3s 4s lt o2)
ports=(ble uart)
conns=(api mqtt)

custom=false

while getopts ":v:t:p:c:" opt; do
  case $opt in
    t)
      types=($OPTARG)
      custom=true
      ;;
    p)
      ports=($OPTARG)
      custom=true
      ;;
    c)
      conns=($OPTARG)
      custom=true
      ;;
    v)
      verbose=-v
      echo "Enabling verbose mode"
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;
  esac
done

shift $((OPTIND-1)) # Shift positional parameters past the options

test_dir=$prj_path/tests/gen
conf_dir=$prj_path/configs

ESPHOME_DATA_DIR=$(realpath $prj_path/.nosync/esphome-tests)

echo "Configuration types: ${types[@]}"
echo "Configuration ports: ${ports[@]}"
echo "Configuration conns: ${conns[@]}"

for type in "${types[@]}"; do
  for port in "${ports[@]}"; do
    for conn in "${conns[@]}"; do
      conf=$test_dir/tion-$type-$port-$conn.yaml
      if [ -f "$conf" ]; then
        esphome $verbose compile $conf || exit
      fi
    done
  done
done

[ "$custom" != "true" ] && esphome compile $verbose $test_dir/tion-multiple.yaml
