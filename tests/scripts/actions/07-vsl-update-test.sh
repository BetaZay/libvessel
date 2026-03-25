#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Testing vsl-update CLI integration"
cd "$VESSEL_ROOT"
$VESSEL_EXE update > /tmp/update_log.txt
cat /tmp/update_log.txt
if ! grep -q "$APP_NAME" /tmp/update_log.txt; then
    echo "TEST FAILED: vsl update did not find registered app: $APP_NAME!"
    exit 1
fi
echo "vsl-update verified."
