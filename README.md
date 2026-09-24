# salama

Web browser for Sailfish OS: a Silica interface over the platform Gecko engine
(`Sailfish.WebView`). Packaged as `harbour-salama`, distributed only through Jolla
Harbour. Targets the Jolla Phone 2026 (`aarch64`) on Sailfish OS 5.2 or newer.
Scope and rules: [SCOPE.md](SCOPE.md). Everything else: [docs/](docs/).

## Build

    make configure    # host toolchain only: Qt 5, CMake, no SDK
    make build
    make test

## Check

    make check          # exactly what CI runs, from a clean checkout, offline
    make fmt-apply      # clang-format in place
    make translations   # refresh translations/*.ts after changing strings
    make sonar-reports  # coverage and compile database for SonarQube Cloud

## Package

    sfdk config target=SailfishOS-<5.2 release>-aarch64
    sfdk build
    sfdk check -s harbour RPMS/harbour-salama-*.aarch64.rpm

Licence: [MPL-2.0](LICENSE). Model and tab logic derives from
[sailfish-browser](https://github.com/sailfishos/sailfish-browser) (Jolla Ltd., MPL-2.0).
The reader view is Firefox's: [Readability](https://github.com/mozilla/readability)
(Mozilla, Apache-2.0, [third_party/readability](third_party/readability)) and a style sheet
adapted from Firefox's `aboutReader.css` (MPL-2.0).
