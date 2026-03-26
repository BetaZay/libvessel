#!/bin/bash
set -e

SCRIPT_DIR="$(dirname "${BASH_SOURCE[0]}")"
export VESSEL_ROOT="${VESSEL_ROOT:-$(realpath "$SCRIPT_DIR/../../../")}" 
export EXAMPLE_DIR="${VESSEL_ROOT}/examples/cmake"
export VESSEL_EXE="${VESSEL_ROOT}/build/vsl"

get_manifest_value() {
  local key=$1
  local json_file=$2
  grep "\"$key\":" "$json_file" | sed -E 's/.*"'"$key"'":\s*"([^"]*)".*/\1/' || echo ""
}

export APP_NAME=$(get_manifest_value "name" "$EXAMPLE_DIR/vessel.json")
export DIST_DIR=$(get_manifest_value "dist_dir" "$EXAMPLE_DIR/vessel.json")
export BUNDLE_NAME="${APP_NAME}.vsl"

if [ -n "$DIST_DIR" ] && [ "$DIST_DIR" != "." ]; then
  export BUNDLE_PATH="${EXAMPLE_DIR}/${DIST_DIR}/${BUNDLE_NAME}"
else
  export BUNDLE_PATH="${EXAMPLE_DIR}/${BUNDLE_NAME}"
fi
