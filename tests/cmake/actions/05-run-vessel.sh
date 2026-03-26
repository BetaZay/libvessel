#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Testing Vessel install-only flow"
chmod +x "$BUNDLE_PATH"
"$BUNDLE_PATH" --install-only
echo "Install-only execution completed."
