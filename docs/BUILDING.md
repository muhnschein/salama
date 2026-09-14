# Building

## Toolchain pins

Host (`make check`, CI): Ubuntu 24.04 packages. Qt 5.15.13 (`qtbase5-dev`,
`qtdeclarative5-dev`, `qtdeclarative5-dev-tools`, `qttools5-dev-tools`,
`libqt5sql5-sqlite`, `qml-module-qtquick2`, `qml-module-qtqml`, `qml-module-qtquick-window2`),
GCC 13, clang-format and clang-tidy 18, CMake 3.28, gcovr 7, shellcheck 0.9,
`desktop-file-utils`, `rpm` (for `rpmspec`), `file`, `librsvg2-bin` (icons only).
The exact `apt-get` line is in `.github/workflows/ci.yml`.

Device: the Sailfish SDK 5.2 with the `SailfishOS-<release>-aarch64` target. Older
SDKs produce a `__libc_start_main` version Harbour rejects; the SDK version is a
Harbour rule. Host Qt is 5.15 while the device has Qt 5.6: the static QML tests and
`ci/qml-lint.sh` catch what 5.15 accepts and 5.6 rejects.

## Lints and gates (`make check`)

| Target | What | Fails on |
|---|---|---|
| `fmt` | clang-format, `.clang-format` | any drift (`make fmt-apply` fixes) |
| `qml-lint` | `ci/qml-lint.sh` | qmllint, console calls, pixel counts, Qt 5.6 syntax, untranslated strings, >400 lines |
| `packaging-lint` | `ci/packaging-lint.sh` | spec, desktop entry, shellcheck, translations compile and current, docs references |
| `harbour-check` | `ci/harbour-check.sh` | any Harbour rule not waived |
| `harbour-selftest` | `ci/harbour-check-selftest.sh` | the checker missing a broken rule |
| `build` | CMake, `-Wall -Wextra -Wpedantic -Werror` | warnings |
| `test` | ctest, one process per test, serial, no retries | any failure |
| `coverage` | gcovr over `src/` (excluding `main.cpp`) | line coverage < 80% |
| `sonar-selftest` | `ci/sonar-report-selftest.sh` against a stub server | the Sonar report script losing a field |
| `tidy` | clang-tidy, `.clang-tidy` | any finding |

Missing tools are SKIP locally and failures in CI (`PACKAGING_LINT_STRICT=1`).

## Test tiers

1. C++ unit tests (`tests/tst_*.cpp`, QtTest) for every model, on temporary directories.
2. QML load tests (`tests/tst_qmlload.cpp`) load the real `qml/` against
   `tests/silica-stubs/` and drive pages by `objectName`. Stubs imitate no layout.
3. Static QML tests (`tests/tst_qmlstatic.cpp`): `Sailfish.WebView` only where §5 allows,
   every `model.<role>` bound by a delegate exists on its model, every singleton member
   referenced exists in C++.
4. Packaging checks (`ci/packaging-lint.sh`).
5. Device smoke test (`TESTING.md`).

## Coverage

`make coverage` writes `build/coverage/sonar-coverage.xml` (SonarQube generic format),
`cobertura.xml` and an HTML report. CI uploads the directory as the `coverage` artifact.

## Static analysis

SonarQube Cloud analyses every pull request from `.github/workflows/sonar.yml`, which is
deliberately not part of `ci.yml`: Sonar is a **report**, `make check` is the gate, and
nothing Sonar says changes a build's colour (SCOPE.md §7).

The scanner measures nothing it can be handed instead. `make sonar-reports` runs
`make coverage` and leaves two files the scan imports, both named in
`sonar-project.properties`:

| File | Why the scanner does not produce it |
|---|---|
| `build/coverage/sonar-coverage.xml` | It only ever imports coverage. Without a report the reading is a confident 0.0%, not "no data". |
| `build/compile_commands.json` | The C++ analyser needs to know how each file is compiled. CMake already writes the database, so no build wrapper is used. |

Analysis must run from CI, not from SonarCloud's **Automatic Analysis**: that mode has no
build step, so it can import neither file and reports 0.0% coverage. The two are mutually
exclusive, and Automatic Analysis has to be off in the SonarCloud project for this
workflow to be accepted.

The scan uploads a report and exits; the server processes it afterwards. `ci/sonar-report.sh`
then asks the server for the quality gate, the measures and the open issues and prints them
into the job log and the step summary, so the result is readable without opening the
dashboard. It is `continue-on-error`: a Sonar outage costs a warning, not a build.
`ci/sonar-report-selftest.sh` runs it against a stub server and is part of `make lint`.

## Device RPM

    sfdk config target=SailfishOS-<release>-aarch64
    sfdk build
    sfdk check -s harbour RPMS/harbour-tuuli-*.aarch64.rpm

`.github/workflows/rpm.yml` does the same unattended, the way postivene's does: a
`docker run` of `coderus/sailfishos-platform-sdk` pinned by digest (5.2.0.15), the
target resolved from the image, the checkout handed to the SDK's own user and mounted
inside its home (rpm under scratchbox2 maps unknown absolute paths into the target
rootfs), then `mb2 -X build-init`, `build-requires` and `build --no-check`. The C++
build runs parallel make on the runner's cores. Nothing is cached on purpose: the only
large input is the SDK image, and restoring it from the Actions cache is no faster than
pulling it; the build itself takes seconds.

Run it from the Actions tab (`sfos_version` is the input), push a `v*` tag for a
release, or a `build-*` tag to build a branch before the workflow reaches the default
branch. The spec keeps `Version: 0.0.0` and `Release: 1`; the workflow stamps the
tag's version and `1.<run number>` so each build installs over the previous one.
The RPM is uploaded as `harbour-tuuli-aarch64-sfos<release>-<sha>` (30 days), and
Jolla's validator then runs on it; a rejection fails the job after the upload.

Without the device SDK the host build links `src/main.cpp` against
`tests/stubs/sailfishapp/` so the entry point still compiles under `-Werror`.
