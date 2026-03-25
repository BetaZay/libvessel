#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Testing vsl-remove CLI integration"
cd "$VESSEL_ROOT"
$VESSEL_EXE remove "$APP_NAME" -y

if grep -q "$APP_NAME" "$HOME/.vessel_registry"; then
    echo "TEST FAILED: vsl remove did not remove $APP_NAME from registry!"
    exit 1
fi

if [ -f "$HOME/.local/share/vessel/bin/$BUNDLE_NAME" ]; then
    echo "TEST FAILED: vsl remove did not delete the installed bundle at $HOME/.local/share/vessel/bin/$BUNDLE_NAME!"
    exit 1
fi

echo "vsl-remove verified."
