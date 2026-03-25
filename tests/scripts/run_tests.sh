#!/bin/bash
set -e

echo "Vessel Native Test Suite Starting..."
echo "=========================================="

VESSEL_ROOT="/workspace/vessel"

# 1. Build the Vessel SDK
echo "--> Compiling the Vessel SDK Platform"
cd $VESSEL_ROOT
rm -rf build
mkdir -p build && cd build
cmake ..
make

echo "Vessel SDK compiled successfully."

# 2. Build the Example Application
echo "--> Compiling the Pong Example"
cd $VESSEL_ROOT/example
rm -rf build

echo "--> Manually Packing Pong using 'vsl pack'"
$VESSEL_ROOT/build/vsl pack

# 3. Verify the .vsl bundle was generated
if [ ! -f "pong.vsl" ]; then
    echo "TEST FAILED: single-file pong.vsl bundle was not generated!"
    exit 1
fi
echo "Validated target executable: pong.vsl"

# 4. Trigger First-Run Registration
echo "--> Testing Vessel Stub Auto-Registration/Extraction"
# We timeout because it's a graphical application running in a headless container
timeout 2 ./pong.vsl || true

# 5. Check the registry
if [ ! -f "$HOME/.vessel_registry" ]; then
    echo "TEST FAILED: ~/.vessel_registry was not created!"
    exit 1
fi
echo "Registration successful. Registry contents:"
cat "$HOME/.vessel_registry"

# 6. Run vsl-update check
echo "--> Testing vsl-update CLI integration"
$VESSEL_ROOT/build/vsl update > /tmp/update_log.txt
cat /tmp/update_log.txt
if ! grep -q "pong" /tmp/update_log.txt; then
    echo "TEST FAILED: vsl update did not find registered app!"
    exit 1
fi

# 7. Test vsl remove
echo "--> Testing vsl-remove CLI integration"
$VESSEL_ROOT/build/vsl remove pong
if grep -q "pong" "$HOME/.vessel_registry"; then
    echo "TEST FAILED: vsl remove did not remove pong from registry!"
    exit 1
fi

if [ -f "$HOME/.local/share/vessel/bin/pong.vsl" ]; then
    echo "TEST FAILED: vsl remove did not delete the installed bundle!"
    exit 1
fi

echo "=========================================="
echo "ALL TESTS PASSED! The Native C++ Vessel ecosystem is fundamentally sound."
