#!/bin/sh
# Rebuilds a release from its source, checks it against the published RPM.
#
#     ci/verify-release.sh [--podman | --docker] [--rpm <file>] [--no-download]
#                          [--ref <ref>] [--save <file>] [--rebuilt <file>] [<version>]
#
#   <version>       release to check, e.g. 0.9.0; default: latest
#   --rpm <file>    check this file too (e.g. one copied off a phone); version from its
#                   name unless given
#   --no-download   don't check GitHub's copy; only --rpm's
#   --ref <ref>     build this tag, branch or commit, not v<version> (rpm.yml checks a
#                   release before it is tagged)
#   --save <file>   keep rebuilt RPM there; with --no-download and no --rpm, only
#                   rebuild (rpm.yml: rebuild beside release build)
#   --rebuilt <file>  don't rebuild: compare against this earlier rebuild
#   --podman, --docker  engine to use; default: Docker if running, else Podman
#
# Needs git, curl, sha256sum, Podman or Docker. First run: ~5 GB pull. Builds in fresh
# clone under SALAMA_WORKDIR, never in this tree; checks for 2 GB free there.
#
# Exit, worst of copies checked: 0 same file, byte for byte; 1 same files inside,
# different packaging (releases before reproducible builds); 2 files differ; 3 couldn't
# check.
#
# Environment, all optional:
#   SALAMA_REPO     where to clone from; default GitHub
#   SALAMA_KEEP     1: keep clone and build
#   SALAMA_WORKDIR  where to clone and build; default ~/.cache/salama-verify (not $TMPDIR:
#                   /tmp often RAM disk, small and shared)
#   build-in-container.sh's SALAMA_* pass through.

set -eu

prog=verify-release
github=muhnschein/salama
repo=${SALAMA_REPO:-https://github.com/$github.git}

usage() {
    sed -n '4,16p' "$0" | sed 's/^# \{0,1\}//' >&2
    exit 3
}

rpm=""
download=1
ref=""
version=""
save=""
rebuilt=""
while [ $# -gt 0 ]; do
    case "$1" in
        --podman | --docker) SALAMA_ENGINE=${1#--}; export SALAMA_ENGINE ;;
        --rpm) [ $# -ge 2 ] || usage; rpm=$2; shift ;;
        --no-download) download=0 ;;
        --ref) [ $# -ge 2 ] || usage; ref=$2; shift ;;
        --save) [ $# -ge 2 ] || usage; save=$2; shift ;;
        --rebuilt) [ $# -ge 2 ] || usage; rebuilt=$2; shift ;;
        -h | --help) usage ;;
        -*) echo "$prog: unknown option $1" >&2; usage ;;
        *) [ -z "$version" ] || usage; version=$1 ;;
    esac
    shift
done

here=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
# shellcheck source=ci/container-engine.sh
. "$here/ci/container-engine.sh"
pick_engine

fail() { echo "$prog: $*" >&2; exit 3; }

# Before build, not during.
need_gb=2
parent=${SALAMA_WORKDIR:-${XDG_CACHE_HOME:-$HOME/.cache}/salama-verify}
mkdir -p "$parent" || fail "can't create $parent"
free_gb=$(df -Pk "$parent" | awk 'NR == 2 { print int($4 / 1048576) }')
[ "${free_gb:-0}" -ge "$need_gb" ] ||
    fail "$parent: ${free_gb:-?} GB free, build needs $need_gb GB; set SALAMA_WORKDIR to a roomier disk"
work=$(mktemp -d "$parent/run.XXXXXX")
# shellcheck disable=SC2317,SC2329 # run by trap.
cleanup() {
    if [ "${SALAMA_KEEP:-0}" = 1 ]; then
        echo "kept: $work"
    else
        rm -rf "$work" 2>/dev/null || echo "$prog: could not remove all of $work" >&2
    fi
}
trap cleanup EXIT
mkdir -p "$work/cmp"

# What to check against: GitHub's copy, --rpm's, or both; none: rebuild only.
[ "$download" = 1 ] || [ -n "$rpm" ] || [ -n "$save" ] ||
    fail "--no-download: nothing to check; give --rpm, or --save to only rebuild"
[ -z "$rebuilt" ] || [ -f "$rebuilt" ] || fail "no such file: $rebuilt"
[ -z "$rebuilt" ] || [ -z "$save" ] || fail "--rebuilt reuses a rebuild, --save keeps a new one; pick one"
if [ -n "$rpm" ]; then
    [ -f "$rpm" ] || fail "no such file: $rpm"
    cp "$rpm" "$work/cmp/local.rpm"
    if [ -z "$version" ]; then
        version=$(basename "$rpm" | sed -n 's/^harbour-salama-\(.*\)-[^-]*\.aarch64\.rpm$/\1/p')
        [ -n "$version" ] || [ -n "$ref" ] ||
            fail "can't read a version from '$(basename "$rpm")'; give one"
    fi
fi
if [ "$download" = 1 ]; then
    # https only, redirects too.
    get() { curl -sSfL --proto '=https' --proto-redir '=https' "$@"; }
    if [ -z "$version" ]; then
        version=$(get "https://api.github.com/repos/$github/releases/latest" |
            sed -n 's/.*"tag_name": *"v\([^"]*\)".*/\1/p' | head -1) ||
            fail "can't ask GitHub for the latest release"
        [ -n "$version" ] || fail "can't find the latest release"
    fi
    url="https://github.com/$github/releases/download/v$version"
    echo ">> downloading release $version"
    get -o "$work/SHA256SUMS" "$url/SHA256SUMS" || fail "no release v$version"
    name=$(awk '/\.rpm$/ { sub(/^\*/, "", $2); print $2; exit }' "$work/SHA256SUMS")
    [ -n "$name" ] || fail "release v$version's SHA256SUMS names no RPM"
    get -o "$work/$name" "$url/$name" || fail "can't download $name"
    (cd "$work" && grep " \*\{0,1\}$name\$" SHA256SUMS | sha256sum -c -) >/dev/null ||
        fail "$name doesn't match the release's SHA256SUMS; download damaged?"
    cp "$work/$name" "$work/cmp/github.rpm"
fi
ref=${ref:-v$version}

if [ -n "$rebuilt" ]; then
    echo ">> comparing against the rebuild in $rebuilt"
    cp "$rebuilt" "$work/cmp/rebuilt.rpm"
    # Image to unpack in: this tree's pin (rpm.yml: same commit).
    src=$here
else
    # Fresh clone: nothing uncommitted from anywhere gets in.
    echo ">> cloning $ref"
    src="$work/harbour-salama"
    if printf '%s\n' "$ref" | grep -Eq '^[0-9a-f]{7,40}$'; then
        git clone --quiet "$repo" "$src"
        git -C "$src" checkout --quiet --detach "$ref"
    else
        git clone --quiet --depth 1 --branch "$ref" "$repo" "$src" 2>/dev/null ||
            git clone --quiet --branch "$ref" "$repo" "$src" ||
            fail "can't clone $ref from $repo"
    fi

    # Source's own recipe. Releases from before it had one: this tree's, same pinned image
    # their rpm.yml used.
    if [ ! -f "$src/ci/build-in-container.sh" ]; then
        echo ">> $ref predates reproducible releases: expect same files inside," \
             "different packaging"
        for f in ci/build-in-container.sh ci/container-engine.sh ci/sdk-images; do
            cp "$here/$f" "$src/$f"
            # Build tooling, not packaged: hide from uncommitted-changes warning.
            echo "/$f" >> "$src/.git/info/exclude"
        done
    fi

    # Commit time, from clone: never caller's.
    unset SOURCE_DATE_EPOCH
    sh "$src/ci/build-in-container.sh" || fail "the build failed"

    built=$(find "$src/RPMS" -name 'harbour-salama-*.rpm' ! -name '*debug*' ! -name '*.src.rpm' | head -1)
    [ -n "$built" ] || fail "the build made no device RPM"
    cp "$built" "$work/cmp/rebuilt.rpm"
    if [ -n "$save" ]; then
        cp "$built" "$save" || fail "can't save the rebuild to $save"
    fi
fi

sum() {
    f=$1
    sha256sum < "$work/cmp/$f.rpm" | cut -d' ' -f1
}
ours=$(sum rebuilt)
echo
for c in github local; do
    [ -f "$work/cmp/$c.rpm" ] && printf '%-8s %s\n' "$c" "$(sum "$c")"
done
printf '%-8s %s\n' rebuilt "$ours"
[ -z "$rpm" ] || echo "local    = $rpm"
if [ ! -f "$work/cmp/github.rpm" ] && [ ! -f "$work/cmp/local.rpm" ]; then
    echo "rebuilt  = $save; nothing to check it against here"
    exit 0
fi

# Unpacked in SDK image (rpm2cpio, cpio there, maybe not here). As root: rootless, that's
# us; reads our files whatever image's user.
image=${SALAMA_SDK_IMAGE:-$(awk -v v="${SALAMA_SFOS:-5.2.0.15}" '$1 == v { print $2; exit }' "$src/ci/sdk-images")}
chmod 755 "$work/cmp"
chmod 644 "$work/cmp"/*.rpm

# One copy against rebuild: 0 same file, 1 same files inside, 2 differ.
check() {
    copy=$1
    [ "$(sum "$copy")" != "$ours" ] || {
        echo "$copy: VERIFIED, same file as the rebuild, byte for byte."
        return 0
    }
    echo ">> $copy: file differs; comparing what's inside"
    rc=0
    # shellcheck disable=SC2016 # expanded by container's shell.
    ctr_run --user 0 -v "$work/cmp:/cmp:ro" "$image" sh -c '
        set -e
        for s in "$1" rebuilt; do
            mkdir -p "/tmp/$s"
            (cd "/tmp/$s" && rpm2cpio "/cmp/$s.rpm" | cpio -idm --quiet)
            (cd "/tmp/$s" && find . -type f -exec sha256sum {} + | sort -k2) > "/tmp/$s.sums"
            rpm -qp --qf "[%{FILEMODES:perms} %{FILEUSERNAME}:%{FILEGROUPNAME} %{FILENAMES} %{FILELINKTOS}\n]" \
                "/cmp/$s.rpm" | sort -k3 > "/tmp/$s.meta"
        done
        echo "$(wc -l < "/tmp/$1.sums") files"
        diff "/tmp/$1.sums" /tmp/rebuilt.sums > /tmp/d1 || true
        diff "/tmp/$1.meta" /tmp/rebuilt.meta > /tmp/d2 || true
        if [ -s /tmp/d1 ] || [ -s /tmp/d2 ]; then
            echo "differing (< $1, > rebuilt):"
            cat /tmp/d1 /tmp/d2 | grep "^[<>]" | head -50
            exit 10
        fi' sh "$copy" || rc=$?
    case "$rc" in
        0) echo "$copy: SAME FILES: every file, mode and owner inside matches the rebuild;"
           echo "   packaging differs (build time, file dates, host or signature)."
           return 1 ;;
        10) echo "$copy: DIFFERENT: not what $ref rebuilds to. Please report it."
            return 2 ;;
        *) fail "couldn't unpack $copy and the rebuild to compare them" ;;
    esac
}

worst=0
for c in github local; do
    [ -f "$work/cmp/$c.rpm" ] || continue
    echo
    rc=0
    check "$c" || rc=$?
    [ "$rc" -le "$worst" ] || worst=$rc
done
echo
case "$worst" in
    0) echo "VERIFIED: $ref rebuilds to every copy checked, byte for byte." ;;
    1) echo "SAME FILES: expected for releases before reproducible builds;" \
            "for a later one, a bug: please report." ;;
    *) echo "DIFFERENT: see above." ;;
esac
exit "$worst"
