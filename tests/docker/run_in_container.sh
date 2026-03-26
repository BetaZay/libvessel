#!/bin/bash
set -euo pipefail

TARGET="${VESSEL_TEST_TARGET:-cmake}"

if [ "$TARGET" != "cmake" ] && [ "$TARGET" != "gradle" ]; then
  echo "Invalid VESSEL_TEST_TARGET '$TARGET' (expected cmake or gradle)"
  exit 1
fi

cd /workspace
bash /workspace/tests/run.sh -t="$TARGET" --local-only
