#!/bin/bash
set -e

# Resolve project root (parent of tests/scripts/actions)
SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
export VESSEL_ROOT="${VESSEL_ROOT:-$(realpath "$SCRIPT_DIR/../../../")}"
export EXAMPLE_DIR="${VESSEL_ROOT}/example"
export VESSEL_EXE="${VESSEL_ROOT}/build/vsl"

# Helper to get value from vessel.json (since we don't have jq in some environments)
get_manifest_value() {
  local KEY=$1
  local JSON_FILE=$2
  grep "\"$KEY\":" "$JSON_FILE" | sed -E 's/.*"'"$KEY"'":\s*"([^"]*)".*/\1/' || echo ""
}

export APP_NAME=$(get_manifest_value "name" "$EXAMPLE_DIR/vessel.json")
export DIST_DIR=$(get_manifest_value "dist_dir" "$EXAMPLE_DIR/vessel.json")
export BUNDLE_NAME="${APP_NAME}.vsl"

# Resolve where the vsl bundle is (could be in current dir or in DIST_DIR)
if [ -n "$DIST_DIR" ] && [ "$DIST_DIR" != "." ] && [ "$DIST_DIR" != "" ]; then
    export BUNDLE_PATH="${EXAMPLE_DIR}/${DIST_DIR}/${BUNDLE_NAME}"
else
    export BUNDLE_PATH="${EXAMPLE_DIR}/${BUNDLE_NAME}"
fi
