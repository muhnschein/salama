#!/bin/bash
# ci/release-notes.sh — print one version's section of docs/CHANGELOG.md, which is the
# text of the GitHub release for that version (docs/RELEASING.md).
#
# Usage: ci/release-notes.sh <version>        # e.g. 0.8.0
#
# A section starts at its Keep a Changelog heading, "## [<version>]" with the date after
# it, and runs to the next "## " heading or the end of the file. The heading itself is
# left out, the release page already has the name, and so are blank lines at either end.
# No such section, or an empty one, is a failure: a release cut before the changelog was
# written stops in the workflow rather than publishing a release with nothing in it.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CHANGELOG=$ROOT/docs/CHANGELOG.md

version=${1:-}
[[ $version =~ ^[0-9]+(\.[0-9]+)*$ ]] || {
    echo "usage: $0 <version>, digits and periods (got '$version')" >&2
    exit 2
}

# The heading is compared as text, not as a pattern, so the version's dots are dots.
# Blank lines are held back until a line follows them, which drops the trailing ones.
notes=$(awk -v head="## [$version]" '
    /^## / { inside = (substr($0, 1, length(head)) == head); next }
    !inside { next }
    /^[ \t]*$/ { if (printed) blanks++; next }
    { while (blanks > 0) { print ""; blanks-- } print; printed = 1 }
' "$CHANGELOG")

[[ -n $notes ]] || {
    echo "release-notes: docs/CHANGELOG.md has no section for $version" >&2
    exit 1
}
printf '%s\n' "$notes"
