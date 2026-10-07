#!/bin/bash
# Run Jolla's validator on built RPM, judge against ci/harbour/waivers.conf.
# Authority ci/harbour-check.sh stands in for: only built package shows generated
# Requires/Provides, stripped symbols, real file modes. Runs in rpm.yml.
#
# Usage: ci/harbour-validate-rpm.sh <rpm>
#
# Validator fetched at ci/harbour/UPSTREAM commit so vendored rules match its code.
# HARBOUR_VALIDATOR points at existing checkout instead.
set -uo pipefail
shopt -s extglob

ROOT=$(cd "$(dirname "$0")/.." && pwd)
UPSTREAM=$ROOT/ci/harbour/UPSTREAM
WAIVERS=$ROOT/ci/harbour/waivers.conf
VALIDATOR=${HARBOUR_VALIDATOR:-/tmp/harbour-validator}

fail() {
    echo "harbour-rpm: FAIL $*" >&2
    exit 1
}

rpm=${1:-}
[[ $# -eq 1 && -n $rpm ]] || { echo "usage: $0 <rpm>" >&2; exit 2; }
[[ -f $rpm ]] || fail "no such RPM: $rpm"

repo=$(sed -n 's/^repo=//p' "$UPSTREAM")
commit=$(sed -n 's/^commit=//p' "$UPSTREAM")
[[ $commit =~ ^[0-9a-f]{40}$ ]] || fail "ci/harbour/UPSTREAM names no commit to pin the validator to"

if [[ ! -x $VALIDATOR/rpmvalidation.sh ]]; then
    { git init -q "$VALIDATOR" \
        && git -C "$VALIDATOR" fetch -q --depth 1 "$repo.git" "$commit" \
        && git -C "$VALIDATOR" checkout -q FETCH_HEAD; } || fail "could not fetch the validator at $commit"
fi

# Vendored rules must equal validator's, else checks disagree.
for conf in "$ROOT"/ci/harbour/*.conf; do
    name=$(basename "$conf")
    [[ $name == waivers.conf ]] && continue
    [[ -f $VALIDATOR/$name ]] || continue
    cmp -s "$conf" "$VALIDATOR/$name" || fail "ci/harbour/$name differs from the validator's; run ci/harbour-allowlists-drift.sh --update"
done

log=$(mktemp)
trap 'rm -f "$log"' EXIT
# BATCHERBATCHERBATCHER: `KIND|subject|message` lines, no colour, `=Section` markers,
# `!END!` verdict. Exits non-zero on warnings too, so markers decide.
BATCHERBATCHERBATCHER=1 "$VALIDATOR/rpmvalidation.sh" -g "$VALIDATOR" "$rpm" >"$log" 2>&1 || true
cat "$log"
grep -q '^!END!' "$log" || fail "the validator produced no verdict"

# Same format as ci/harbour-check.sh: `<check-id> <subject-glob> <message-glob>`;
# validator findings use `rpm-<section>` (e.g. rpm-requires).
waived() { # id subject message
    local id=$1 subject=$2 message=$3 entry wid wsubject wmessage
    [[ -f $WAIVERS ]] || return 1
    while IFS= read -r entry; do
        entry=${entry%%#*}
        read -r wid wsubject wmessage <<<"$entry"
        [[ -n ${wid:-} && -n ${wsubject:-} && -n ${wmessage:-} ]] || continue
        # shellcheck disable=SC2053
        [[ $id == "$wid" && $subject == $wsubject && $message == $wmessage ]] && return 0
    done <"$WAIVERS"
    return 1
}

errors=0
waivedCount=0
section=rpm
while IFS= read -r line; do
    case "$line" in
        =*)
            section=$(tr '[:upper:] ' '[:lower:]-' <<<"${line#=}")
            section="rpm-$section"
            ;;
        ERROR\|*)
            subject=$(cut -d'|' -f2 <<<"$line")
            message=$(cut -d'|' -f3- <<<"$line")
            if waived "$section" "$subject" "$message"; then
                echo "harbour-rpm: WAIVED [$section] $subject -- $message"
                waivedCount=$((waivedCount + 1))
            else
                echo "harbour-rpm: FAIL [$section] $subject -- $message" >&2
                errors=$((errors + 1))
            fi
            ;;
        WARNING\|*)
            echo "harbour-rpm: warning [$section] $(cut -d'|' -f2 <<<"$line") -- $(cut -d'|' -f3- <<<"$line")"
            ;;
    esac
done <"$log"

echo
if [[ $errors -gt 0 ]]; then
    echo "harbour-rpm: FAILED -- $errors finding(s) Harbour would reject, $waivedCount waived" >&2
    exit 1
fi
if [[ $waivedCount -gt 0 ]]; then
    echo "harbour-rpm: ok -- nothing new; $waivedCount waived finding(s) still block submission (docs/HARBOUR.md)"
else
    echo "harbour-rpm: ok -- the validator accepts this package"
fi
