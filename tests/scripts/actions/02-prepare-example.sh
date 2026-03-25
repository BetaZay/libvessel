#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Preparing the Pong Example"
cd "$EXAMPLE_DIR"
rm -rf build
if [ -n "$DIST_DIR" ] && [ "$DIST_DIR" != "." ] && [ "$DIST_DIR" != "" ]; then
    rm -rf "$DIST_DIR"
fi
rm -f "$BUNDLE_NAME"
echo "Example environment cleaned."
