#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Testing Vessel extraction mode for Gradle bundle"
cd "$EXAMPLE_DIR"
chmod +x "$BUNDLE_PATH"
"$BUNDLE_PATH" --vsl-extract

UNPACK_DIR="$EXAMPLE_DIR/${APP_NAME}_vsl_unpacked"
if [ ! -f "$UNPACK_DIR/bin/VesselRun" ]; then
    echo "TEST FAILED: missing VesselRun in extracted bundle ($UNPACK_DIR/bin/VesselRun)."
    exit 1
fi

if [ ! -f "$UNPACK_DIR/bin/$BIN_FILE_NAME" ]; then
    echo "TEST FAILED: missing expected JAR in extracted bundle ($UNPACK_DIR/bin/$BIN_FILE_NAME)."
    exit 1
fi

echo "Gradle extraction verified."
