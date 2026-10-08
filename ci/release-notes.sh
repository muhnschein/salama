#!/bin/bash
# Print one version's docs/CHANGELOG.md section = GitHub release text (docs/RELEASING.md).
#
# Usage: ci/release-notes.sh <version>        # e.g. 0.8.0
#
# Section: from "## [<version>]" heading to next "## " or EOF. Heading and edge blank
# lines dropped. Missing or empty section fails, so release without notes stops.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CHANGELOG=$ROOT/docs/CHANGELOG.md

version=${1:-}
[[ $version =~ ^[0-9]+(\.[0-9]+)*$ ]] || {
    echo "usage: $0 <version>, digits and periods (got '$version')" >&2
    exit 2
}

# Heading compared as text, not pattern (dots are dots).
# Blank lines held until a line follows; drops trailing ones.
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
