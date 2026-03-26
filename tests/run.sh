#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TESTS_DIR="$ROOT_DIR/tests"
DOCKER_DIR="$TESTS_DIR/docker"

TARGET="all"
USE_DOCKER=false
LOCAL_ONLY=false

usage() {
  cat <<USAGE
Usage: tests/run.sh [-t=cmake|gradle|all] [--docker]

Options:
  -t=<target>   Select test target: cmake, gradle, or all (default: all)
  --docker      Run the selected target(s) inside ubuntu/fedora/arch containers
  --local-only  Internal flag used by container runner
  -h, --help    Show this help message
USAGE
}

for arg in "$@"; do
  case "$arg" in
    -t=*)
      TARGET="${arg#-t=}"
      ;;
    --docker)
      USE_DOCKER=true
      ;;
    --local-only)
      LOCAL_ONLY=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg"
      usage
      exit 1
      ;;
  esac
done

case "$TARGET" in
  cmake|gradle|all) ;;
  *)
    echo "Invalid target '$TARGET'. Use cmake, gradle, or all."
    exit 1
    ;;
esac

if [ "$LOCAL_ONLY" = true ] && [ "$USE_DOCKER" = true ]; then
  echo "--local-only cannot be used with --docker"
  exit 1
fi

run_local_target() {
  local target="$1"
  local suite="$TESTS_DIR/$target/run_tests.sh"

  if [ ! -x "$suite" ]; then
    chmod +x "$suite"
  fi

  echo "Running local test suite: $target"
  "$suite"
}

run_docker_target() {
  local target="$1"
  local os_list=("ubuntu" "fedora" "arch")

  echo "Running Docker matrix for target: $target"
  echo "===================================="

  for os in "${os_list[@]}"; do
    local image="vessel-test-${os}-${target}"
    local dockerfile="$DOCKER_DIR/${os}.Dockerfile"

    echo "Building $os image for target '$target'..."
    docker build -t "$image" -f "$dockerfile" "$ROOT_DIR"

    echo "Running $os container for target '$target'..."
    docker run --rm -e VESSEL_TEST_TARGET="$target" "$image"

    echo "Platform '$os' passed for target '$target'."
    echo "------------------------------------"
  done
}

run_selected_targets() {
  local mode="$1"
  local targets

  if [ "$TARGET" = "all" ]; then
    targets=("cmake" "gradle")
  else
    targets=("$TARGET")
  fi

  for target in "${targets[@]}"; do
    if [ "$mode" = "docker" ]; then
      run_docker_target "$target"
    else
      run_local_target "$target"
    fi
  done
}

if [ "$USE_DOCKER" = true ]; then
  run_selected_targets "docker"
else
  run_selected_targets "local"
fi

echo "All requested test suites completed successfully."
