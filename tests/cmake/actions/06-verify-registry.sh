#!/bin/bash
set -e
SCRIPT_DIR="$(dirname "$0")"
source "$SCRIPT_DIR/common.sh"

echo "--> Checking the registry"
if [ ! -f "$HOME/.vessel_registry" ]; then
    echo "TEST FAILED: ~/.vessel_registry was not created!"
    exit 1
fi
echo "Registration successful. Registry contents:"
cat "$HOME/.vessel_registry"
