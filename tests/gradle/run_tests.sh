#!/bin/bash
set -e

echo "Vessel Gradle Test Suite Starting..."
echo "=========================================="

SCRIPT_DIR="$(dirname "$0")"
ACTIONS_DIR="$SCRIPT_DIR/actions"

chmod +x "$ACTIONS_DIR"/*.sh

"$ACTIONS_DIR/01-build-sdk.sh"
"$ACTIONS_DIR/02-prepare-example.sh"
"$ACTIONS_DIR/03-pack.sh"
"$ACTIONS_DIR/04-verify-bundle.sh"
"$ACTIONS_DIR/05-verify-extract.sh"

echo "=========================================="
echo "Gradle test suite passed."
