#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Compiling the Vessel SDK Platform"
cd "$VESSEL_ROOT"
rm -rf build
mkdir -p build && cd build
cmake ..
make

echo "Vessel SDK compiled successfully."
