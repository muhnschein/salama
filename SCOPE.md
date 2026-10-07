# SCOPE.md — salama

Web browser for Sailfish OS. Silica UI over the platform Gecko engine via `Sailfish.WebView`. Package name `harbour-salama`. Distributed only through Jolla Harbour.

## 1. Goal

A browser with a contemporary interface, reusing the platform browser stack unmodified. This is a UI project. The engine is not in scope.

## 2. Target

- **Device:** Jolla Phone 2026 only. `aarch64` only.
- **OS:** Sailfish OS 5.2 or newer. `Requires: sailfish-version >= 5.2.0`.
- **SDK:** the 5.2 SDK. Older SDKs produce a `__libc_start_main` version Harbour rejects; the SDK version is therefore a Harbour rule, not a preference.

No effort is made for other or older hardware, `armv7hl`, `i486`, or the emulator. Code paths, layouts, and tests exist for one screen and one architecture.

## 3. Non-goals

- Building, patching, or bundling Gecko/xulrunner.
- Supporting community `-next` engine stacks. If Jolla ships a newer engine, salama inherits it.
- Distribution via Chum, OpenRepos, or side-loaded RPMs.
- WebExtensions.
- Content blocking beyond what `WebEngineSettings` exposes.
- Multi-architecture or multi-device support.
- Any language other than QML and C++.

## 4. Constraints

| Constraint | Consequence |
|---|---|
| Harbour allowed-API list | Only `Sailfish.WebView`, `.Controls`, `.Popups`, `.Pickers`, `Sailfish.WebEngine`, Silica, and listed Qt/Nemo modules. No private `Sailfish.Browser` plugin. |
| Engine version tracks OS | Web-platform feature set is not ours to change. |
| Qt 5.6 | No newer Qt/QML APIs. Host Qt 5.15 accepts what 5.6 rejects; see §7 static QML tests. |
| `harbour-` namespace, Sailjail | All storage under the app data dir. `[X-Sailjail]` permissions minimal and listed in `docs/HARBOUR.md`. |
| MPL-2.0 (sailfish-browser) | Ported code stays MPL-2.0 with attribution preserved. Project licence: MPL-2.0. |

## 5. Architecture

```
qml/           Silica UI. Root, cover/, pages/, components/.
src/           C++ core. QObject / QAbstractListModel types exposed to QML.
  tabs/        TabModel, TabPersistence
  history/     HistoryModel (SQLite)
  bookmarks/   BookmarkModel (SQLite)
  settings/    Settings and a section per settings page (QSettings)
  startpage/   StartPage: what a tab with no address shows
  reader/      Reader: the reader view, and Firefox's style sheet for it
  notifications/
               NotificationPermissions, WebNotifications: the pages' notifications
  permissions/ SitePermissions, SiteExceptions: what each site may do
  share/       ShareReceiver: links shared to the browser from the share sheet
third_party/   Readability (Mozilla, Apache-2.0), verbatim
tests/         QtTest units, QML load tests, silica-stubs/, static QML tests
ci/            harbour-check.sh, harbour-check-selftest.sh, packaging-lint.sh,
               qml-lint.sh, harbour/ (validator allow-lists, waivers.conf)
rpm/           harbour-salama.spec
docs/          See §8
```

Reuse policy:
- Platform, unmodified: WebView, text selection, JS/auth/permission dialogs, file pickers, download plumbing.
- Ported from sailfish-browser: engine-independent C++ model and tab-container logic.
- From Firefox: the reader view -- Readability as published, `aboutReader.css` adapted; web notifications -- Firefox's question and answers, kept in the engine's permissions.
- New: all UI.

`Sailfish.WebView` is imported in the browsing page only, so a release without the engine package breaks browsing rather than the app.

## 6. Engineering standards

Adopted from postivene and vuo. The governing rule: **`make check` runs exactly what CI runs, from a clean checkout, with no phone, no SDK, and no network.** Anything that cannot be verified under those conditions is badly layered or sits behind an explicit opt-in gate.

### Code
- C++14, `-Wall -Wextra -Wpedantic -Werror`. `clang-tidy` with a checked-in config; findings are errors.
- `clang-format` checked in; `make fmt` fails on drift.
- `qmllint` clean. No `console.log` in shipped QML. `Theme` values, never pixel counts.
- Engine quirks are isolated in C++ with a comment naming the upstream issue. None in QML.
- One responsibility per QML file.
- No dead code, no commented-out code, no TODO without an issue number.
- Every user-visible string translatable; catalogs current and compiling.

### Tests
1. **C++ unit tests** (QtTest) for every model. Coverage of `src/` ≥ 80%, enforced in CI.
2. **QML load tests** against `tests/silica-stubs/`: the real page files, driven by `objectName`. Stubs imitate no layout; these prove structure, not appearance.
3. **Static QML tests**: Qt 5.6 rules that host Qt accepts silently; `Sailfish.WebView` imported only where §5 says; every `model.<role>` a delegate binds exists on its model.
4. **Packaging checks** (`ci/packaging-lint.sh`): spec parses, desktop entry validates, shell scripts clean, translations compile, every `docs/*.md` a comment points at exists. Missing tool is SKIP locally, failure in CI (`PACKAGING_LINT_STRICT=1`).
5. **Device smoke test** before every tag, run under `sailjail /usr/bin/harbour-salama`, never from the IDE. Checklist in `docs/TESTING.md`.

Tests run one per process. No retries: a test that passes on the second attempt is a defect.
A bug fix includes a regression test or a written justification in the PR.

### Harbour gate
Two checks, kept honest against each other:

| | `ci/harbour-check.sh` | `sfdk check -s harbour` |
|---|---|---|
| Runs | every pull request | when the RPM is built |
| Reads | source tree | built package |
| Authority | no | **yes** |

`ci/harbour-check.sh` reimplements Jolla's `rpmvalidation.sh` logic over the sources: naming, install layout, desktop file, Sailjail keys and permissions, icons, QML imports against the allow-list, linked libraries, RPM metadata, runtime path policy. `ci/harbour/` holds the validator's allow-lists copied verbatim; a CI step warns when they lag upstream. `ci/harbour-check-selftest.sh` breaks each rule in a throwaway tree and asserts the check names it. Anything not in `ci/harbour/waivers.conf` fails.

### Static analysis
SonarQube Cloud on every pull request. A **report, not a gate**: `make check` decides what merges; nothing Sonar says changes a build's colour. Coverage is measured locally and imported, not measured by the scanner.

### Process
- `main` always releasable. Feature branches, squash merge, linear history.
- PR requires: green CI, one review, changelog entry.
- Commit subject imperative, ≤ 72 chars; body says why.
- Semantic versioning. The version is the spec's; CI builds, validates and publishes each release from a `v` tag on `main`.
- Dependencies: Harbour allowed list only. Any addition updates `docs/HARBOUR.md` in the same PR.

## 7. Documentation

Kept in `docs/`. Updated in the PR that changes the subject. No document duplicates another.

| File | Content | Limit |
|---|---|---|
| `README.md` | Build, check, package in three commands each | 1 page |
| `ARCHITECTURE.md` | Module boundaries, data flow, storage schema | 2 pages |
| `HARBOUR.md` | Jolla's rules, how CI gates them, current waivers, Sailjail permissions and why each | 2 pages |
| `BUILDING.md` | Toolchain pins, lints, test tiers, how a device RPM is built | 2 pages |
| `RELEASING.md` | Tag, build, validate, submit | 1 page |
| `CHANGELOG.md` | Keep-a-Changelog, user-facing entries only | — |

Not maintained: design narratives, roadmaps beyond this file, tutorials, marketing copy.