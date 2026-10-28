#!/bin/bash
# clang-tidy over all own C++ via host build compile DB. Findings are errors.
#
# One clang-tidy per file, TIDY_JOBS at once (default: cores): serial run was ~7 min of
# CI's ~12. Output per file, printed whole only when that file fails: no interleaving.
set -euo pipefail

BUILD_DIR=${1:-build}
JOBS=${TIDY_JOBS:-$(nproc 2>/dev/null || echo 2)}
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
LOGS=$(mktemp -d)
trap 'rm -rf "$LOGS"' EXIT

# tidy_one <build dir> <logs dir> <file>: findings to <logs>/<file>.log; clean file leaves
# <file>.ok instead. File with neither never ran (xargs stopped early): not clean either.
# shellcheck disable=SC2016 # expanded by inner bash.
printf '%s\0' "${SOURCES[@]}" |
    xargs -0 -P "$JOBS" -n 1 bash -c '
        out=$2/$(printf %s "$3" | tr / _)
        if clang-tidy --quiet -p "$1" "$3" >"$out.log" 2>&1; then
            mv "$out.log" "$out.ok"
        else
            exit 1
        fi' tidy_one "$BUILD_DIR" "$LOGS" || true

mapfile -t FAILED < <(find "$LOGS" -name '*.log' | sort)
CLEAN=$(find "$LOGS" -name '*.ok' | wc -l)
if [[ ${#FAILED[@]} -gt 0 ]]; then
    cat "${FAILED[@]}"
    echo "clang-tidy: findings in ${#FAILED[@]} of ${#SOURCES[@]} files" >&2
    exit 1
fi
if [[ $CLEAN -ne ${#SOURCES[@]} ]]; then
    echo "clang-tidy: only $CLEAN of ${#SOURCES[@]} files checked" >&2
    exit 1
fi
echo "clang-tidy: ${#SOURCES[@]} files clean"
