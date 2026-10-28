#!/usr/bin/env bash
# Does this change need build, tests, tidy and Sonar? Prints `code=true` or
# `code=false` for $GITHUB_OUTPUT (`changes` job in ci.yml, sonar.yml).
#
# false only for a PR touching nothing but `*.md` outside third_party/ (CMake
# installs third_party/readability/LICENSE.md). Docs PRs still get
# packaging-lint: it reads docs/CHANGELOG.md, docs/HARBOUR.md, every docs/*.md
# link. Push to main, dispatch, a diff that can't be read: true. Running too
# much costs minutes; skipping wrongly costs a red main.
#
# Usage:  ci/changes.sh                 (PR: diff of merge commit vs base)
#         ci/changes.sh --classify      (file names on stdin; for selftest)
#
# EVENT = github.event_name.
set -euo pipefail

# File names on stdin; docs only = every one `*.md`, none under third_party/. Nothing named = true (empty diff is no proof).
classify() {
    local file seen=0
    while IFS= read -r file; do
        [[ -n "$file" ]] || continue
        seen=1
        case "$file" in
        third_party/*)
            echo code=true
            return 0
            ;;
        *.md) ;;
        *)
            echo code=true
            return 0
            ;;
        esac
    done
    if [[ "$seen" -eq 0 ]]; then
        echo code=true
    else
        echo code=false
    fi
    return 0
}

if [[ "${1:-}" = --classify ]]; then
    classify
    exit 0
fi

event=$(printenv EVENT || true)
if [[ "$event" != pull_request ]]; then
    echo code=true
    exit 0
fi

# PR checkout = merge commit: first parent = base, so this is the PR's change.
if ! files=$(git diff --name-only HEAD^1 HEAD); then
    echo "changes: diff unreadable; running everything" >&2
    echo code=true
    exit 0
fi
classify <<<"$files"
