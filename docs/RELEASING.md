# Releasing

Semantic versioning, `main` always releasable. A release is `.github/workflows/rpm.yml`
run for a version on `main`, as piirit's and vuo's are.

1. Put the version in `rpm/harbour-salama.spec` (`Version:`) and in `CMakeLists.txt`
   (`project(... VERSION ...)`), and move the `[Unreleased]` entries under
   `## [X.Y.Z] - YYYY-MM-DD` in `docs/CHANGELOG.md`, leaving `[Unreleased]` empty above
   it. `make check` fails when the two versions differ or the changelog has no section
   for the spec's. Merge that to `main`.
2. Cut it, either way:
   - dispatch `rpm` on `main` from the Actions tab with `X.Y.Z` in `release`, which
     tags the commit `vX.Y.Z` itself; or
   - tag the merge commit by hand, `git tag -s vX.Y.Z -m "salama X.Y.Z"`, and push the
     tag, which runs the same workflow.
3. The workflow refuses a version the spec does not say (and a dispatch off `main`),
   builds the `aarch64` RPM with the spec's own `Release: 1`, runs Jolla's validator on
   it, and only then publishes the GitHub release `vX.Y.Z`: the device RPM, a
   `SHA256SUMS`, and the version's changelog section as its text
   (`ci/release-notes.sh`, which fails the run if the section is missing). The debug
   and source RPMs stay on the run's artifact.
4. Validate: the device smoke test in `TESTING.md` on the RPM from the release page.
   Locally, `sfdk build` and `sfdk check -s harbour RPMS/harbour-salama-X.Y.Z-1.aarch64.rpm`
   build and judge the same package.
5. Submit the RPM from the release page at https://harbour.jolla.com with the changelog
   section as release notes.
6. After approval, note the Harbour release date in the changelog.

Never re-tag: a rejected submission gets a new patch version.
