#!/bin/bash
set -e

# Go to the workspace Root
cd "$(dirname "$0")/.."

ROOT_DIR="$(pwd)"

echo "Vessel Multi-OS Test Orchestrator"
echo "===================================="

OS_LIST=("ubuntu" "fedora" "arch")

for OS in "${OS_LIST[@]}"; do
    echo "Building Test Image for $OS..."
    docker build -t vessel-test-$OS -f $ROOT_DIR/tests/docker/$OS.Dockerfile .

    echo "Running Integration Suite on $OS..."
    docker run --rm vessel-test-$OS
    
    echo "Platform '$OS' fully verified."
    echo "------------------------------------"
done

echo "ALL OS PLATFORMS PASSED NATIVE COMPILATION AND PACKAGING!"
