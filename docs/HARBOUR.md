# Harbour

Harbour only channel, so Jolla's rules are build rules.

## Jolla's rules, CI gates

Two checks, kept honest against each other:

| | `ci/harbour-check.sh` | `sfdk check -s harbour` |
|---|---|---|
| Runs | every PR (`make check`) | on RPM build (`rpm` workflow) |
| Reads | source tree | built package |
| Authority | no | **yes** |

`ci/harbour-check.sh` reimplements Jolla's `rpmvalidation.sh` (sdk-harbour-rpmvalidator)
over sources: package name, RPM metadata (version, release, vendor, scriptlets, triggers,
provides, dependency types), QML imports vs allow-list, desktop file + `[X-Sailjail]` keys
and permissions, install layout (spec `%files`, CMake destinations), icons, linked
libraries, exported `main()`, link flags, `Requires`, hardcoded paths, runtime path policy.
`ci/harbour-check-selftest.sh` breaks each rule in throwaway tree, asserts check names it.

`ci/harbour/*.conf`: validator allow-lists verbatim; `ci/harbour/UPSTREAM` pins source and
commit. `rpm` workflow runs validator from same commit on built package
(`ci/harbour-validate-rpm.sh`), so both checks read same rules. `allow-lists` job in
`.github/workflows/ci.yml` warns when upstream moves; `ci/harbour-allowlists-drift.sh --update`
refreshes files and pin together. Lists are GPL-2.0-or-later; CI reads them as data, app
never links them.

Anything not in `ci/harbour/waivers.conf` fails, both checks. Waiver = check id
(`requires`, `qml-import`, ... source check; `rpm-requires`, `rpm-paths`, ... validator
sections), subject, message, all globs, reason as comment.

## Current waivers

None.

## Sailjail permissions

`harbour-salama.desktop`, `[X-Sailjail]`:

| Permission | Why |
|---|---|
| `Internet` | engine network, favicons |
| `WebView` | Gecko embedding: `/usr/share/mozilla`, transfer engine for downloads (needed by any `Sailfish.WebView` user) |
| `Audio` | page sound. `Base` profile blocks PulseAudio (`nosound`) without it; `WebView` lacks it, so engine plays silent. Also admits mic at PulseAudio level; recording gated by `Microphone` |
| `Downloads` | engine saves to `~/Downloads/Salama`, app creates folder |
| `Pictures` | photo upload via platform picker |
| `Videos` | video upload via platform picker |
| `Music` | audio upload via platform picker |
| `Documents` | document upload via platform picker |
| `MediaIndexing` | picker's Images/Videos/Music/Documents lists are Tracker queries; without `org.freedesktop.Tracker3.Miner.Files` they're empty, only File system left. Jolla's browser holds it for same reason |
| `Location` | page `navigator.geolocation`; sandbox admits engine position only with this. Without it Site permissions offers useless choice |
| `Camera` | page `getUserMedia` video; sandbox gates camera on this |
| `Microphone` | page `getUserMedia` audio; needed beside `Audio`, site must still be allowed |

`ExecDBus=harbour-salama` lets system start browser for share sheet call on D-Bus name
`io.github.muhnschein.salama`. Desktop file's one share method, `link`, takes `text/x-url`
only; answered via `QtDBus` (on validator list).

`OrganizationName=io.github.muhnschein`, `ApplicationName=salama` define writable data,
cache, config dirs; nothing else stored elsewhere except downloads. Sharing needs no
permission (`Base`). Notifications neither: `Base` includes `Notifications.permission`
for `org.freedesktop.Notifications`. Shown via `Nemo.Notifications 1.0` (validator QML
list); package requires `nemo-qml-plugin-notifications-qt5` (validator dependency list).
Upload picker is Sailfish.Pickers' `ContentPickerPage`: each list needs its folder
permission plus `MediaIndexing`; verified in device smoke test.

## Runtime path policy

Writes only `QStandardPaths::AppDataLocation`, `AppConfigLocation`, `CacheLocation`, plus
`Salama` folder in `DownloadLocation` (app creates, engine saves to). `QSettings` always
gets explicit file path (sailjail-permissions README).
`ci/harbour-check.sh` fails on other standard locations and on `/home/nemo` or
`/home/defaultuser` literals.
