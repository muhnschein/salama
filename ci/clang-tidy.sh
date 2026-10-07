#!/bin/bash
# clang-tidy over all our C++ via host build compile DB. Findings are errors.
set -euo pipefail

BUILD_DIR=${1:-build}
cd "$(dirname "$0")/.."

if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
    echo "clang-tidy: $BUILD_DIR/compile_commands.json missing; run 'make configure' first" >&2
    exit 1
fi

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy: not installed" >&2
    exit 1
fi

mapfile -t SOURCES < <(find src tests -name '*.cpp' | sort)
clang-tidy --quiet -p "$BUILD_DIR" "${SOURCES[@]}"
echo "clang-tidy: ${#SOURCES[@]} files clean"
