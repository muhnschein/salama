#!/bin/sh
# Builds device RPM in pinned Sailfish SDK image via Podman or Docker (no installed SDK).
#
#     ci/build-in-container.sh [<sfos-version>]
#
# Version default: SALAMA_SFOS, else 5.2.0.15, Jolla Phone 2026 baseline. Image: one ci/sdk-images pins for
# it, by digest. rpm.yml runs this too: release = same steps as anyone's build.
#
# Reproducible: SOURCE_DATE_EPOCH (default: commit time) sets package build time and file
# dates; host name and build path fixed. Same commit, same image: same RPM, byte for byte
# (docs/VERIFYING.md).
#
# Environment, all optional:
#   SALAMA_ENGINE     docker | podman; default: Docker if running, else Podman
#   SALAMA_SDK_IMAGE  image to use instead of pinned one
#   SALAMA_SFOS       SDK version when none given (verify-release.sh passes it through)
#
# Rootless Podman: no root anywhere, tree stays yours. Docker: tree chowned to image's user
# for build, back after (root in container, no host sudo).
#
# aarch64 only (SCOPE.md §2).

set -eu

prog=build-in-container
sfos=${1:-${SALAMA_SFOS:-5.2.0.15}}
arch=aarch64

case "$sfos" in
    *[!0-9.]* | '') echo "$prog: '$sfos' is not a dotted SDK version" >&2; exit 2 ;;
    *) ;;
esac

repo_root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"

# shellcheck source=ci/container-engine.sh
. "$repo_root/ci/container-engine.sh"
pick_engine
echo ">> using $engine$([ "$rootless" = 1 ] && echo ' (rootless)')"

image=${SALAMA_SDK_IMAGE:-$(awk -v v="$sfos" '$1 == v { print $2; exit }' ci/sdk-images)}
[ -n "$image" ] || {
    echo "$prog: no image pinned for $sfos in ci/sdk-images; set SALAMA_SDK_IMAGE" >&2
    exit 1
}

# Untracked or changed files still get packaged (install globs qml/, translations/): result
# won't match a release. Warn, don't refuse: rpm.yml stamps Release into spec.
if [ -z "${SOURCE_DATE_EPOCH:-}" ]; then
    SOURCE_DATE_EPOCH=$(git log -1 --format=%ct 2>/dev/null || true)
fi
if [ -z "$SOURCE_DATE_EPOCH" ]; then
    echo "$prog: warning: not a git checkout and no SOURCE_DATE_EPOCH;" \
         "package dates will be today's, so it won't match a release" >&2
else
    dirty=$(git status --porcelain 2>/dev/null || true)
    if [ -n "$dirty" ]; then
        echo "$prog: warning: these changes aren't committed; they go into the" \
             "package, so it won't match a release:" >&2
        echo "$dirty" | head -10 >&2
    fi
fi

# mb2 builds in tree; spec's `build/` would reuse host's CMake cache and fail minutes in.
[ ! -f build/CMakeCache.txt ] || {
    echo "$prog: build/ holds a host build (make check); run 'make clean' first" >&2
    exit 1
}

ctr_pull "$image"

# Ask image for target, don't assume: odd naming fails here with list, not in rpmbuild.
targets=$(ctr_run "$image" sb2-config -l 2>/dev/null || true)
target=$(echo "$targets" | grep -- "-$arch\$" | tail -1 || true)
[ -n "$target" ] || {
    echo "$prog: no $arch target in $image; it has:" >&2
    echo "$targets" >&2
    exit 1
}

# sdk-manage refuses root ("cannot determine the Mer SDK user"): run as image's user, uid
# read from image, not assumed.
uid=$(ctr_run "$image" id -u)
gid=$(ctr_run "$image" id -g)
# shellcheck disable=SC2016 # image's $HOME, not ours.
home=$(ctr_run "$image" sh -c 'echo $HOME')

# In SDK user's home, named for spec. scratchbox2 maps unknown absolute paths into target
# rootfs: tree at /build written outside, read inside. mb2 takes package name from dir
# (rpm/<name>.spec); ci/packaging-lint.sh checks it. Fixed path: debug info records it.
builddir="$home/harbour-salama"

# Rootless Podman: keep-id maps us onto image's user, mounted tree already theirs. Else
# chown there and back.
keep_id=0
[ "$engine" = podman ] && [ "$rootless" = 1 ] && keep_id=1

in_image() {
    if [ "$keep_id" = 1 ]; then
        set -- --userns=keep-id:uid="$uid",gid="$gid" "$@"
    fi
    ctr_run -v "$repo_root:$builddir" "$@"
}
hand_to() {
    # Plain assignment: POSIX sh has no `local`.
    owner=$1
    in_image --user root "$image" chown -R "$owner" "$builddir"
}
# Also on failure, else host user can't read or delete build output. Rootless Docker:
# container root is us.
give_back() {
    if [ "$rootless" = 1 ]; then hand_to 0:0 || true
    else hand_to "$(id -u):$(id -g)" || true
    fi
}
if [ "$keep_id" = 0 ]; then
    trap give_back EXIT
    hand_to "$uid:$gid"
fi

# `-X` on build-init too: else it asks `git describe`, finds no tag, never writes
# .mb2/spec. build-requires pulls BuildRequires from Jolla's repos for this release.
# --privileged: as SDK's own instructions. Fixed host name: rpm records it (spec pins
# %_buildhost too).
echo ">> building in $image, target $target"
# shellcheck disable=SC2016 # expanded by container's shell.
in_image --privileged --hostname salama-build -w "$builddir" \
    -e TARGET="$target" \
    ${SOURCE_DATE_EPOCH:+-e SOURCE_DATE_EPOCH="$SOURCE_DATE_EPOCH"} "$image" \
    /bin/bash -c 'set -e
        mb2 -t "$TARGET" -X build-init
        mb2 -t "$TARGET" -X build-requires ||
            echo "build-in-container: mb2 build-requires failed; the build may stop on a missing BuildRequires" >&2
        mb2 -t "$TARGET" -X build --no-check'

if [ "$keep_id" = 0 ]; then
    give_back
    trap - EXIT
fi

echo
echo "RPMs:"
find RPMS -name '*.rpm' 2>/dev/null || echo "  (none under RPMS/)"
