# SCOPE.md — salama

Sailfish OS web browser. Silica UI over platform Gecko via `Sailfish.WebView`. Package `harbour-salama`. Jolla Harbour only.

## 1. Goal

Contemporary UI on unmodified platform browser stack. UI project. Engine out of scope.

## 2. Target

- **Device:** Jolla Phone 2026 only. `aarch64` only.
- **OS:** Sailfish OS 5.2+. `Requires: sailfish-version >= 5.2.0`.
- **SDK:** 5.2. Older SDKs emit `__libc_start_main` version Harbour rejects, so SDK version is Harbour rule.

No work for other/older hardware, `armv7hl`, `i486`, emulator. One screen, one arch.

## 3. Non-goals

- Building, patching, bundling Gecko/xulrunner.
- Community `-next` engine stacks. Newer Jolla engine inherited as-is.
- Chum, OpenRepos, side-loaded RPMs.
- WebExtensions.
- Content blocking beyond `WebEngineSettings`.
- Multi-arch, multi-device.
- Languages other than QML, C++.

## 4. Constraints

| Constraint | Consequence |
|---|---|
| Harbour allowed-API list | Only `Sailfish.WebView`, `.Controls`, `.Popups`, `.Pickers`, `Sailfish.WebEngine`, Silica, listed Qt/Nemo modules. No private `Sailfish.Browser` plugin. |
| Engine version tracks OS | Web-platform features not ours. |
| Qt 5.6 | No newer Qt/QML APIs. Host Qt 5.15 accepts what 5.6 rejects; static QML tests (§6) catch it. |
| `harbour-` namespace, Sailjail | Storage under app data dir. `[X-Sailjail]` permissions minimal, listed in `docs/HARBOUR.md`. |
| MPL-2.0 (sailfish-browser) | Ported code stays MPL-2.0, attribution kept. Project licence MPL-2.0. |

## 5. Architecture

```
qml/           Silica UI. Root, cover/, pages/, components/.
src/           C++ core. QObject / QAbstractListModel types for QML.
  tabs/        TabModel, TabPersistence
  history/     HistoryModel (SQLite)
  bookmarks/   BookmarkModel (SQLite)
  settings/    Settings, one section per settings page (QSettings)
  startpage/   StartPage: what empty tab shows
  reader/      Reader view, Firefox style sheet
  notifications/
               NotificationPermissions, WebNotifications: page notifications
  permissions/ SitePermissions, SiteExceptions: per-site permissions
  share/       ShareReceiver: links from share sheet
third_party/   Readability (Mozilla, Apache-2.0), verbatim
tests/         QtTest units, QML load tests, silica-stubs/, static QML tests
ci/            harbour-check.sh, harbour-check-selftest.sh, packaging-lint.sh,
               qml-lint.sh, harbour/ (validator allow-lists, waivers.conf)
rpm/           harbour-salama.spec
docs/          See §7
```

Reuse:
- Platform, unmodified: WebView, text selection, JS/auth/permission dialogs, file pickers, download plumbing.
- From sailfish-browser: engine-independent C++ models, tab-container logic.
- From Firefox: reader view (Readability as published, `aboutReader.css` adapted); web notification question and answers, stored in engine permissions.
- New: all UI.

`Sailfish.WebView` imported in browsing page only: missing engine package breaks browsing, not app.

## 6. Engineering standards

From postivene and vuo. Rule: **`make check` = CI, from clean checkout, no phone, no SDK, no network.** Anything unverifiable that way is mislayered or behind explicit opt-in.

### Code
- C++14, `-Wall -Wextra -Wpedantic -Werror`. `clang-tidy`, checked-in config, findings are errors.
- `clang-format` checked in; `make fmt` fails on drift.
- `qmllint` clean. No `console.log` in shipped QML. `Theme` values, never pixel counts.
- QML file ≤ 400 lines. Waivers listed in `ci/qml-lint.sh`, with reason; nothing over 600.
- Engine quirks isolated in C++, comment names upstream issue. None in QML.
- One responsibility per QML file.
- No dead code, no commented-out code, no TODO without issue number.
- Every user-visible string translatable; catalogs current, compiling.
- Comments terse: why, not what. No narrative.

### Tests
1. **C++ unit tests** (QtTest) per model. `src/` coverage ≥ 80%, enforced in CI.
2. **QML load tests** against `tests/silica-stubs/`: real page files, driven by `objectName`. Stubs have no layout; prove structure, not looks.
3. **Static QML tests**: Qt 5.6 rules host Qt accepts silently; `Sailfish.WebView` only where §5 says; every `model.<role>` delegate binds exists on its model.
4. **Packaging checks** (`ci/packaging-lint.sh`): spec parses, desktop entry valid, shell scripts clean, translations compile, every `docs/*.md` a comment names exists. Missing tool: SKIP locally, fail in CI (`PACKAGING_LINT_STRICT=1`).
5. **Device smoke test** before every tag, under `sailjail /usr/bin/harbour-salama`, never from IDE.

One test per process. No retries: pass on second attempt = defect.
Bug fix brings regression test or written justification in PR.

### Harbour gate
Two checks, kept honest against each other:

| | `ci/harbour-check.sh` | `sfdk check -s harbour` |
|---|---|---|
| Runs | every PR | on RPM build |
| Reads | source tree | built package |
| Authority | no | **yes** |

`ci/harbour-check.sh` reimplements Jolla's `rpmvalidation.sh` over sources: naming, install layout, desktop file, Sailjail keys/permissions, icons, QML imports vs allow-list, linked libraries, RPM metadata, runtime paths. `ci/harbour/` holds validator allow-lists verbatim; CI step warns when they lag upstream. `ci/harbour-check-selftest.sh` breaks each rule in throwaway tree, asserts check names it. Anything not in `ci/harbour/waivers.conf` fails.

### Static analysis
SonarQube Cloud on every PR. **Report, not gate**: `make check` decides merges. Coverage measured locally, imported.

### Process
- `main` always releasable. Feature branches, squash merge, linear history.
- PR needs: green CI, one review, changelog entry.
- Commit subject imperative, ≤ 72 chars; body says why.
- Semver. Version lives in spec; CI builds, validates, publishes each release from `v` tag on `main`.
- Dependencies: Harbour allowed list only. Addition updates `docs/HARBOUR.md` in same PR.

## 7. Documentation

In `docs/`. Updated in PR that changes subject. No duplication.

| File | Content | Limit |
|---|---|---|
| `README.md` | Build, check, package, three commands each | 1 page |
| `HARBOUR.md` | Jolla rules, CI gates, waivers, Sailjail permissions + why | 2 pages |
| `BUILDING.md` | Toolchain pins, lints, test tiers, device RPM build | 2 pages |
| `RELEASING.md` | Tag, build, validate, submit | 1 page |
| `TRANSLATING.md` | Filling a catalog | 1 page |
| `CHANGELOG.md` | Keep-a-Changelog, user-facing entries only | — |

Not kept: design narratives, roadmaps beyond this file, tutorials, marketing copy.
