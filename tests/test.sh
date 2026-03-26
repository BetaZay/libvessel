#!/bin/bash
set -e

cd "$(dirname "$0")/.."
bash ./tests/run.sh --docker -t=all
