# Harbour

Harbour is the only distribution channel, so Jolla's rules are build rules.

## Jolla's rules, and how CI gates them

Two checks, kept honest against each other:

| | `ci/harbour-check.sh` | `sfdk check -s harbour` |
|---|---|---|
| Runs | every pull request (`make check`) | when the RPM is built (`rpm` workflow) |
| Reads | source tree | built package |
| Authority | no | **yes** |

`ci/harbour-check.sh` reimplements the logic of Jolla's `rpmvalidation.sh`
(sdk-harbour-rpmvalidator) over the sources: package name, RPM metadata (version,
release, vendor, scriptlets, triggers, provides, dependency types), QML imports against
the allow-list, the desktop file and its `[X-Sailjail]` keys and permissions, install
layout (spec `%files` and CMake destinations), icons, linked libraries, exported
`main()` and link flags, `Requires`, hardcoded paths and the runtime path policy.
`ci/harbour-check-selftest.sh` breaks every one of those rules in a throwaway tree and
asserts the check names it.

`ci/harbour/*.conf` are the validator's allow-lists copied verbatim; `ci/harbour/UPSTREAM`
records the source and the commit. The `rpm` workflow runs the validator itself, from
that commit, on the built package (`ci/harbour-validate-rpm.sh`), so the two checks read
the same rules. The `allow-lists` job in `.github/workflows/ci.yml` warns when upstream
has moved on; `ci/harbour-allowlists-drift.sh --update` refreshes files and pin together.
The lists carry the validator's GPL-2.0-or-later licence; CI reads them as data, the
application never links them.

Anything not in `ci/harbour/waivers.conf` fails, in both checks. A waiver names the
check id (`requires`, `qml-import`, ... for the source check; `rpm-requires`,
`rpm-paths`, ... for the validator's sections), the subject and the message, all as
globs, with the reason as a comment.

## Current waivers

None.

## Deviations from SCOPE.md

- `Requires: sailfish-version >= 5.2.0` is not used: the validator rejects any
  dependency outside `allowed_requires.conf`, and `sailfish-version` is not there. The OS
  floor is carried by the SDK target used to build (Harbour checks the
  `__libc_start_main` version the 5.2 toolchain produces) and by the minimum versions of
  allowed packages in the spec. Verify those minimums against the 5.2.0 release before
  Phase 1 (`DECISIONS/0007-os-floor.md`).

## Sailjail permissions

`harbour-salama.desktop`, `[X-Sailjail]`:

| Permission | Why |
|---|---|
| `Internet` | network access for the engine and favicon images |
| `WebView` | Gecko embedding: `/usr/share/mozilla`, the transfer engine for downloads (required for any `Sailfish.WebView` user) |
| `Audio` | sound from pages: Sailjail's `Base` profile shuts every application out of PulseAudio (`nosound`) unless it holds this, and `WebView` does not include it, so without it the engine plays video and audio in silence. It also admits the microphone at the PulseAudio level; nothing here records, and the `Microphone` permission, which recording is meant to need, is not asked for |
| `Downloads` | the engine saves downloads to `~/Downloads/Salama`, a folder the application creates (`DECISIONS/0025-downloads-folder.md`) |
| `Pictures` | uploading a photo through the platform picker in web forms |
| `Documents` | uploading a document through the platform picker |

`OrganizationName=io.github.muhnschein`, `ApplicationName=salama` define the writable
data, cache and config directories; apart from downloads, nothing is stored anywhere
else. Sharing needs no permission (part of the `Base` set), and neither do notifications:
`Base` includes `Notifications.permission`, which lets an application talk to
`org.freedesktop.Notifications` (`DECISIONS/0033-web-notifications.md`). They are shown
through `Nemo.Notifications 1.0`, on the validator's list of QML imports, and the package
requires `nemo-qml-plugin-notifications-qt5`, on its list of dependencies. Whether the pickers need more
than `Pictures` and `Documents` is SCOPE.md §9 item 5 and is verified on the device smoke
test.

## Runtime path policy

Only `QStandardPaths::AppDataLocation`, `AppConfigLocation` and `CacheLocation` are
written, plus the `Salama` folder in `DownloadLocation`, which the application creates and
the engine saves downloads to; `QSettings` always gets an explicit file path
(sailjail-permissions README).
`ci/harbour-check.sh` fails on other standard locations and on `/home/nemo` or
`/home/defaultuser` literals.
