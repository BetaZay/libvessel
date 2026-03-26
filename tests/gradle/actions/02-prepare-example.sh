#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Preparing the Gradle example"
cd "$EXAMPLE_DIR"
rm -rf dist
rm -rf app/build
rm -rf "${APP_NAME}_vsl_unpacked"
rm -f "$BUNDLE_NAME"
echo "Example environment cleaned."
