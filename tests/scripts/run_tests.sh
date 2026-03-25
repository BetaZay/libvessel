#!/bin/bash
set -e

echo "Vessel Native Test Suite Starting..."
echo "=========================================="

SCRIPT_DIR="$(dirname "$0")"
ACTIONS_DIR="$SCRIPT_DIR/actions"

# Make actions executable
chmod +x "$ACTIONS_DIR"/*.sh

"$ACTIONS_DIR/01-build-sdk.sh"
"$ACTIONS_DIR/02-prepare-example.sh"
"$ACTIONS_DIR/03-pack.sh"
"$ACTIONS_DIR/04-verify-bundle.sh"
"$ACTIONS_DIR/05-run-vessel.sh"
"$ACTIONS_DIR/06-verify-registry.sh"
"$ACTIONS_DIR/07-vsl-update-test.sh"
"$ACTIONS_DIR/08-vsl-remove-test.sh"

echo "=========================================="
echo "ALL TESTS PASSED! The Native C++ Vessel ecosystem is fundamentally sound."
