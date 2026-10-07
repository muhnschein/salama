# Building

## Toolchain pins

Host (`make check`, CI): Ubuntu 24.04. Qt 5.15.13 (`qtbase5-dev`, `qtdeclarative5-dev`,
`qtdeclarative5-dev-tools`, `qttools5-dev-tools`, `libqt5sql5-sqlite`, `qml-module-qtquick2`,
`qml-module-qtqml`, `qml-module-qtquick-window2`), GCC 13, clang-format/clang-tidy 18,
CMake 3.28, gcovr 7, shellcheck 0.9, `desktop-file-utils`, `rpm` (`rpmspec`), `file`,
`librsvg2-bin` (icons only). Exact `apt-get` line: `.github/workflows/ci.yml`.

Device: Sailfish SDK 5.2, target `SailfishOS-<release>-aarch64`. Older SDKs emit
`__libc_start_main` version Harbour rejects. Host Qt 5.15, device Qt 5.6: static QML tests
and `ci/qml-lint.sh` catch what 5.15 accepts and 5.6 rejects.

## Lints and gates (`make check`)

| Target | What | Fails on |
|---|---|---|
| `fmt` | clang-format, `.clang-format` | drift (`make fmt-apply` fixes) |
| `qml-lint` | `ci/qml-lint.sh` | qmllint, console calls, pixel counts, Qt 5.6 syntax, untranslated strings, >400 lines |
| `packaging-lint` | `ci/packaging-lint.sh` | spec, desktop entry, shellcheck, translations warn or stale, dead docs refs |
| `harbour-check` | `ci/harbour-check.sh` | unwaived Harbour rule |
| `harbour-selftest` | `ci/harbour-check-selftest.sh` | checker misses broken rule |
| `build` | CMake, `-Wall -Wextra -Wpedantic -Werror` | warnings |
| `test` | ctest, one process per test, serial, no retries | any failure |
| `coverage` | gcovr over `src/` (minus `main.cpp`) | line coverage < 80% |
| `sonar-selftest` | `ci/sonar-report-selftest.sh` vs stub server | report script drops field |
| `tidy` | clang-tidy, `.clang-tidy` | any finding |

Missing tool: SKIP locally, fail in CI (`PACKAGING_LINT_STRICT=1`).

## Test tiers

1. C++ unit tests (`tests/tst_*.cpp`, QtTest) per model, on temp dirs.
2. QML load tests (`tests/tst_qmlload.cpp`): real `qml/` against `tests/silica-stubs/`,
   pages driven by `objectName`. Stubs have no layout.
3. Static QML tests (`tests/tst_qmlstatic.cpp`): `Sailfish.WebView` only where SCOPE.md §5
   allows, every delegate `model.<role>` exists on its model, every singleton member exists in C++.
4. Packaging checks (`ci/packaging-lint.sh`), catalog completeness
   (`tests/tst_translations.cpp`, `docs/TRANSLATING.md`).
5. Device smoke test.

## Coverage

`make coverage` writes `build/coverage/sonar-coverage.xml` (SonarQube generic),
`cobertura.xml`, HTML report. CI uploads dir as `coverage` artifact.

## Static analysis

SonarQube Cloud runs per PR from `.github/workflows/sonar.yml`, kept out of `ci.yml` on
purpose: Sonar is **report**, `make check` is gate (SCOPE.md §6).

Scanner imports, not measures. `make sonar-reports` runs `make coverage`, leaves two files
named in `sonar-project.properties`:

| File | Why scanner can't make it |
|---|---|
| `build/coverage/sonar-coverage.xml` | Scanner only imports coverage. No report = 0.0%, not "no data". Exception branches (632 of 1732, untestable) dropped, else imported figure sits ~20 points under line coverage. |
| `build/compile_commands.json` | C++ analyser needs compile flags. CMake writes DB, no build wrapper. |

Run from CI, not SonarCloud **Automatic Analysis**: that mode has no build step, imports
neither file, reports 0.0%. Modes exclusive: Automatic Analysis must be off.

Scan uploads and exits; server processes later. `ci/sonar-report.sh` then fetches quality
gate, measures, open issues into job log and step summary. `continue-on-error`: Sonar outage
= warning, not red build. `ci/sonar-report-selftest.sh` tests it against stub server (`make lint`).

## Device RPM

    sfdk config target=SailfishOS-<release>-aarch64
    sfdk build
    sfdk check -s harbour RPMS/harbour-salama-*.aarch64.rpm

`.github/workflows/rpm.yml` does same unattended (postivene's pattern): `docker run` of
`coderus/sailfishos-platform-sdk` pinned by digest (5.2.0.15), target from image, checkout
owned by SDK user and mounted in its home (scratchbox2 rpm maps unknown absolute paths into
target rootfs), then `mb2 -X build-init`, `build-requires`, `build --no-check`. Parallel make.
No cache: SDK image restore no faster than pull; build takes seconds.

Run from Actions tab (`sfos_version` input), cut release with it (`RELEASING.md`), or push
`build-*` tag to build branch before workflow reaches default branch. Spec holds version and
`Release: 1`. Release builds exactly that and publishes; other builds stamped
`1.<run number>` so each installs over previous.
RPM uploaded as `harbour-salama-aarch64-sfos<release>-<sha>` (30 days), then Jolla's
validator runs; rejection fails job after upload.

Without device SDK, host build links `src/main.cpp` against `tests/stubs/sailfishapp/` so
entry point still compiles under `-Werror`.
