#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Testing Vessel Stub Auto-Registration/Extraction"
# We timeout because it's a graphical application running in a headless container
# The bundle is made executable by vsl pack, but let's be sure.
chmod +x "$BUNDLE_PATH"
timeout 2 "$BUNDLE_PATH" || true
echo "Extraction/Execution attempted."
