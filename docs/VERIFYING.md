# Verifying a release

Each release RPM rebuilds from its tag to same file, byte for byte. Check one:

```
$ git clone https://github.com/muhnschein/salama.git
$ cd salama
$ ci/verify-release.sh          # latest release
$ ci/verify-release.sh 0.9.0    # or given one
```

Needs Linux on x86_64, git, curl, Podman (rootless fine) or Docker. Both installed: Docker
if running; `--podman` or `--docker` picks. First run: ~5 GB pull of Sailfish SDK image
(`coderus/sailfishos-platform-sdk`, pinned by digest in `ci/sdk-images`), few minutes'
build. Builds in fresh clone of tag under `~/.cache/salama-verify`, never in your tree;
checks for 2 GB free. Elsewhere: `SALAMA_WORKDIR=/some/big/disk`. Not `/tmp`: often RAM
disk.

| Result | Exit | Meaning |
|---|---|---|
| `VERIFIED` | 0 | Same file as published. |
| `SAME FILES` | 1 | Every file inside matches; packaging (build time, file dates, host) differs. Expected for 0.8.0 and earlier, which predate reproducible builds. |
| `DIFFERENT` | 2 | Files differ. Please open issue with output. |

Rootless Podman: SDK image's build user is uid 100000, beyond the 65536 subordinate ids a
user gets by default. Once:

```
$ sudo usermod --add-subuids 100000-599999 --add-subgids 100000-599999 $USER
$ podman system migrate
```

## RPM from elsewhere

E.g. one copied off phone. Checked with GitHub's in one run: one build, each copy
compared, line per copy; exit is worst.

```
$ ci/verify-release.sh --rpm harbour-salama-0.9.0-1.aarch64.rpm
github: VERIFIED, same file as the rebuild, byte for byte.
local: VERIFIED, same file as the rebuild, byte for byte.
```

`--no-download`: that copy only.

## What makes it reproducible

- Package build time, file dates: commit time (`SOURCE_DATE_EPOCH`, set by
  `ci/build-in-container.sh`); spec clamps to it. Build host fixed (spec, container).
- Build environment: SDK image pinned by digest (`ci/sdk-images`); same build path in it.
- Inputs: all in git. No submodules, no downloads. BuildRequires already in pinned image:
  `mb2 build-requires` finds each installed.

`ci/packaging-lint.sh` (`make check`) fails if spec drops the date and host pins.

Releases pass this check in CI before publishing (`reproduce` and `compare` in
`.github/workflows/rpm.yml`: fresh clone, Podman, other runner, time zone, umask). `main`
weekly and PRs touching packaging too (`.github/workflows/reproducible.yml`).

## What it doesn't cover

- Sailfish SDK's compilers and libraries: you trust Jolla's SDK and coderus's image of it,
  as every Sailfish app does.
- macOS, Windows, ARM hosts: untested. Image is x86_64.
