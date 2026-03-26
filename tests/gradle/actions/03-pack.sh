#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Packing Gradle app using 'vsl pack'"
cd "$EXAMPLE_DIR"
"$VESSEL_EXE" pack
