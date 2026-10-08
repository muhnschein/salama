#!/bin/bash
# Packaging checks (SCOPE.md §6, test tier 4):
#  * spec parses (rpmspec)
#  * desktop entry validates (desktop-file-validate)
#  * shell scripts clean (shellcheck)
#  * translations compile, current (lrelease, lupdate)
#  * every docs/*.md a comment names exists
#  * changelog has Unreleased; Sailjail permissions documented
#  * one version: CMake = spec, changelog has its section
#
# Missing tool: SKIP locally, fail with PACKAGING_LINT_STRICT=1 (CI).
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
STRICT=${PACKAGING_LINT_STRICT:-0}
FAILED=0
SPEC=$ROOT/rpm/harbour-salama.spec
DESKTOP=$ROOT/harbour-salama.desktop

fail() { # check-id location message
    local id=$1 location=$2 message=$3
    printf 'ERROR [%s] [%s] %s\n' "$id" "$location" "$message"
    FAILED=1
    return 0
}

have() { # tool
    local tool=$1
    if command -v "$tool" >/dev/null 2>&1; then
        return 0
    fi
    if [[ $STRICT == 1 ]]; then
        fail tool-missing "$tool" "required in CI (PACKAGING_LINT_STRICT=1)"
    else
        echo "SKIP  [$tool] not installed"
    fi
    return 1
}

# 1. Spec parses
if have rpmspec; then
    rpmspec -P "$SPEC" >/dev/null 2>&1 || fail spec rpm/harbour-salama.spec "$(rpmspec -P "$SPEC" 2>&1 | head -1)"
fi

# 2. Desktop entry validates
if have desktop-file-validate; then
    desktop-file-validate "$DESKTOP" || fail desktop harbour-salama.desktop "desktop-file-validate failed"
fi

# 3. Shell scripts clean
if have shellcheck; then
    mapfile -t scripts < <(find "$ROOT/ci" "$ROOT/icons" -name '*.sh' | sort)
    shellcheck --severity=style "${scripts[@]}" || fail shellcheck ci/ "shellcheck reported findings"
fi

# 4. Translations compile cleanly and are current
TS_SOURCE=$ROOT/translations/harbour-salama.ts
sources_of() { # catalogue
    local ts=$1
    grep -o '<source>[^<]*</source>' "$ts" | sort -u
    return 0
}
# Compiled as RPM does; any warning fails (missing plural form or stray tag = English on phone).
if have lrelease; then
    tmp=$(mktemp -d)
    for ts in "$ROOT"/translations/*.ts; do
        if ! out=$(lrelease -silent "$ts" -qm "$tmp/$(basename "${ts%.ts}").qm" 2>&1); then
            fail translations "translations/$(basename "$ts")" "lrelease failed: $out"
        elif grep -qi warning <<<"$out"; then
            fail translations "translations/$(basename "$ts")" "lrelease warns: $out"
        fi
    done
    rm -rf "$tmp"
fi
for ts in "$ROOT"/translations/*.ts; do
    [[ $ts == "$TS_SOURCE" ]] && continue
    diff -q <(sources_of "$TS_SOURCE") <(sources_of "$ts") >/dev/null || fail translations "translations/$(basename "$ts")" "source strings differ from harbour-salama.ts; run 'make translations'"
done
# Current: `make translations` output must equal committed catalogs. Else new qsTr()
# silently missing from forty languages.
if have lupdate; then
    tmp=$(mktemp -d)
    cp "$ROOT"/translations/*.ts "$tmp/"
    # Own sources only, not page scripts src/reader/reader.qrc compiles in.
    if (cd "$ROOT" && lupdate -silent -no-obsolete -locations none -extensions cpp,h,qml qml src -ts "$tmp"/*.ts >/dev/null 2>&1); then
        for ts in "$ROOT"/translations/*.ts; do
            cmp -s "$ts" "$tmp/$(basename "$ts")" || fail translations "translations/$(basename "$ts")" "catalog is stale; run 'make translations' and commit"
        done
    else
        fail translations translations/ "lupdate failed"
    fi
    rm -rf "$tmp"
fi

# 5. Every docs/*.md a comment points at exists
while read -r ref; do
    [[ -f $ROOT/$ref ]] || fail docs-reference "$ref" "referenced but missing"
done < <(grep -rhoE 'docs/[A-Za-z0-9_./-]+\.md' "$ROOT/src" "$ROOT/qml" "$ROOT/tests" "$ROOT/ci" "$ROOT/rpm" "$ROOT/docs" "$ROOT/README.md" "$ROOT/Makefile" "$ROOT/CMakeLists.txt" 2>/dev/null | sort -u)

# 6. Changelog and permission documentation
grep -q '^## \[Unreleased\]' "$ROOT/docs/CHANGELOG.md" 2>/dev/null || fail changelog docs/CHANGELOG.md "missing '## [Unreleased]' section"
permissions=$(sed -n 's/^Permissions=//p' "$DESKTOP" | tr ';' ' ')
for permission in $permissions; do
    grep -q "\b$permission\b" "$ROOT/docs/HARBOUR.md" 2>/dev/null || fail permissions docs/HARBOUR.md "Sailjail permission '$permission' is not documented"
done

# 7. One version. Spec's is release version (docs/RELEASING.md); CMake must match, changelog
#    must have its section, so bump without notes stops here.
spec_version=$(sed -n 's/^Version:[[:space:]]*//p' "$SPEC")
cmake_version=$(sed -n 's/^project(harbour-salama VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")
[[ $cmake_version == "$spec_version" ]] || fail version CMakeLists.txt "project VERSION '$cmake_version' is not the spec's Version '$spec_version'"
"$ROOT/ci/release-notes.sh" "$spec_version" >/dev/null || fail changelog docs/CHANGELOG.md "no '## [$spec_version]' section for the spec's Version"

if [[ $FAILED -eq 0 ]]; then
    echo "packaging-lint: clean"
    exit 0
fi
exit 1
