#!/bin/bash

set -e

source $(dirname $0)/post-create-env

sedi() {
  expr=$1
  file=$2
  # Failed on VirtioFS, so workaraund it
  # sed -i "$expr" "$file"
  temp=$(mktemp)
  sed "$expr" "$file" > $temp && mv $temp $file
}

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

files=($pio_ini .clang-format .clang-tidy .editorconfig pylintrc)
for f in "${files[@]}" ; do
  #if [ ! -f "$f" ] || [ "$f" = "$pio_ini" ]; then
    curl -Ls "https://github.com/esphome/esphome/raw/dev/$f" -o ".esphome/$f"
    ln -svf ".esphome/$f" "$f"
  #fi
done

pio_ini=".esphome/$pio_ini"

# replace "esphome" to "." in src_dir. esphome linked at post-start.sh
sedi "/src_dir/s/esphome/\./" "$pio_ini"
# replace ".temp" to ".esphome in "sdkconfig_path"
sedi "/sdkconfig_path/s/.temp/.esphome/" "$pio_ini"


sedi "s/-DESPHOME_LOG_LEVEL/\${prj.build_flags}\n    -DESPHOME_LOG_LEVEL/" "$pio_ini"
sedi "s/esphome\/noise-c@/\${prj.lib_deps}\n    esphome\/noise-c@/" "$pio_ini"

echo "" >> "$pio_ini"
echo "# project specific settings" >> "$pio_ini"
echo "[prj]" >> "$pio_ini"
echo "build_flags =" >> "$pio_ini"
for d in "${defines[@]}" ; do
  echo "    -D$d" >> "$pio_ini"
done
echo "lib_deps =" >> "$pio_ini"
for d in "${lib_deps[@]}" ; do
  echo "    $d" >> "$pio_ini"
done

cpp_json=.vscode/c_cpp_properties.json
#if [ ! -f $cpp_json ]; then
if [ "$(find /esphome/.temp/platformio/* -maxdepth 0 -type d 2>/dev/null|wc -l)" -lt "1" ]; then
    pio init --ide vscode --silent -e esp32-arduino
    sedi "/\\/workspaces\/esphome\/include/d" $cpp_json
    rm -rf CMakeLists.txt components/CMakeLists.txt
else
    echo "Cpp environment already configured. To reconfigure it you could run one the following commands:"
    echo "  pio init --ide vscode -e esp8266-arduino"
    echo "  pio init --ide vscode -e esp32-arduino"
    echo "  pio init --ide vscode -e esp32-idf"
fi

# additionally remove annoying pio ide recomendations
sedi "/platformio.platformio-ide/d" .vscode/extensions.json
