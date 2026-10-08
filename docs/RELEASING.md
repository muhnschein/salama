# Releasing

Semver, `main` always releasable. Release = `.github/workflows/rpm.yml` run for a version
on `main` (as piirit, vuo).

1. Set version in `rpm/harbour-salama.spec` (`Version:`) and `CMakeLists.txt`
   (`project(... VERSION ...)`). Move `[Unreleased]` entries under
   `## [X.Y.Z] - YYYY-MM-DD` in `docs/CHANGELOG.md`, leave `[Unreleased]` empty above.
   `make check` fails if versions differ or changelog lacks spec's section. Merge to `main`.
2. Cut, either:
   - dispatch `rpm` on `main` from Actions tab, `X.Y.Z` in `release`; workflow tags
     `vX.Y.Z`; or
   - tag merge commit by hand, `git tag -s vX.Y.Z -m "salama X.Y.Z"`, push tag (same workflow).
3. Workflow refuses version spec doesn't say (and dispatch off `main`), builds `aarch64`
   RPM with spec's `Release: 1`, runs Jolla's validator, then publishes GitHub release
   `vX.Y.Z`: device RPM, `SHA256SUMS`, changelog section as text (`ci/release-notes.sh`,
   fails if section missing). Debug and source RPMs stay on run artifact.
4. Validate: device smoke test on release page RPM. Locally `sfdk build` and
   `sfdk check -s harbour RPMS/harbour-salama-X.Y.Z-1.aarch64.rpm` judge same package.
5. Submit RPM from release page at https://harbour.jolla.com, changelog section as notes.
6. After approval, note Harbour release date in changelog.

Never re-tag: rejected submission gets new patch version.
