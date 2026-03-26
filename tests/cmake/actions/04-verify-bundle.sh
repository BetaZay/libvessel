#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Verifying if the .vsl bundle was generated"
if [ ! -f "$BUNDLE_PATH" ]; then
    echo "TEST FAILED: single-file $BUNDLE_NAME bundle was not generated at $BUNDLE_PATH!"
    exit 1
fi
echo "Validated target executable: $BUNDLE_PATH"
