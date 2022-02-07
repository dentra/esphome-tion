#!/bin/bash

set -e

# take ownership of mounted folders
files=(.esphome .pio)
for f in "${files[@]}" ; do
  if [ ! -d "$f" ]; then
    mkdir "$f"
  else
    sudo chown -R vscode:vscode "$f"
  fi
done

pio_ini=platformio.ini

files=($pio_ini .clang-format .clang-tidy .editorconfig)

for f in "${files[@]}" ; do
  if [ ! -f "$f" ]; then
    curl -Ls "https://github.com/esphome/esphome/raw/dev/$f" -o ".esphome/$f"
    ln -sv ".esphome/$f" "$f"
  fi
done


# replace "esphome" to "." in src_dir. esphome linked at post-start.sh
sed -i -e "/src_dir/s/esphome/\./" $pio_ini
# replace ".temp" to ".esphome in "sdkconfig_path"
sed -i -e "/sdkconfig_path/s/.temp/.esphome/" $pio_ini

cpp_json=.vscode/c_cpp_properties.json
if [ ! -f $cpp_json ]; then
    pio init --ide vscode --silent -e esp32
    sed -i "/\\/workspaces\/esphome\/include/d" $cpp_json
    rm CMakeLists.txt components/CMakeLists.txt
else
    echo "Cpp environment already configured. To reconfigure it you could run one the following commands:"
    echo "  pio init --ide vscode -e esp8266"
    echo "  pio init --ide vscode -e esp32"
    echo "  pio init --ide vscode -e esp32-idf"
fi

# additionally remove annoying pio ide recomendations
sed -i "/platformio.platformio-ide/d" .vscode/extensions.json

