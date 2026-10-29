# AGENTS.md

How to work on Salama. What it is: `README.md`, `SCOPE.md`, `docs/`. Read `SCOPE.md` and `docs/BUILDING.md` before first change. User's words in chat beat this file.

## Ground rules

- Don't know how to proceed → say "I don't know", stop, ask. Mark guesses as guesses. Unknown beats confident wrong.
- Bugs: find root cause, state it. Symptom fix ≠ fix.
- Good return on cost. No gold-plating, no unasked refactors.
- Options → lay them out, recommend one with reasons. Implement nothing until user clearly picks.
- Engine is platform's (SCOPE.md §1, §3). Feature not doable through APIs Sailfish OS exposes → say so, drop or defer. Never patch, bundle or work around Gecko.

## Done means

In this order; later step only once earlier ones hold:

1. Change implemented.
2. Tests added for it (regression test per bug).
3. Tests proven to bite: each fails with change reverted or mutated. Report how (e.g. "22/22 mutations caught"); name what headless tests can't prove.
4. `make check` green locally.
5. PR CI green (`check` workflow). SonarQube is report, not gate (SCOPE.md §6), but read it: no new issues, new-code coverage held. Say what it flags.
6. Device: dispatch `.github/workflows/rpm.yml` on PR branch; run RPM on test phone (see "Device tests"); give user artifact link and evidence.
7. With RPM: outline briefly what merging this PR risks and how you mitigated it.

PR opens as draft on first push. Mark ready for review only once steps 1–7 hold and their evidence is in PR body.

Feature reachable from several places (page menu, tab view, long-press sheet, cover, settings, share sheet) → all of them.

User tests RPM on phone; that test judges feel, lag, transitions. Say exactly what to check on device.

## Device tests

- Test phone: OnePlus 6, SFOS 5.1, via `muhnschein-sfos-harness` workflow `do`, shared with other apps' agents. Its AGENTS.md rules every run (lease, tags, baseline, no blind taps, costly ops); helpers: its README, `scripts/lib.sh`; Salama's screens: its `apps/salama.md`. Never run fork or others' PR code there: any job can root phone.
- Phone runs SFOS 5.1, Salama needs 5.2: harness installs past dependencies (user's go, until phone has 5.2). UI, settings, storage results hold; engine behaviour (loads, rendering, Gecko crashes) may differ on 5.2: report as 5.1 result.
- Build in: `install_artifact muhnschein/salama RUN_ID harbour-salama` (rpm.yml run on PR branch) or `install_rpm URL SHA256` (release). Same job as `launch`: harness refuses build it didn't install.
- Every run starts from named state, `state.txt` records it plus RPM: `clean` (`reset`), `fixture:NAME`, `upgrade:FROM` (release FROM plus data, then build over it; required when change touches stored data or settings).
- Footprint check (`fs_mark`, `footprint harbour-salama`): file written outside known dirs → add to harness `apps/harbour-salama.dirs` or fail run.
- Before/after: same start state, same steps. Motion or timing change: launch with `FRAMELOG`, report `frames`. Never measure frames while recording.
- `text` (harness keyboard) closes address field: no page loads via address bar yet (harness `apps/salama.md` "Typing").
- Evidence: only files from harness results branch, as published (crops, GIFs, charts made from them OK); never type identifiers in. Harness proves function and frame timing; feel, lag, transitions stay user's call on device.

## Local checks

- `make check` = CI: clean checkout, no phone, no SDK, no network. Targets and what each catches: BUILDING.md "Lints and gates".
- Missing optional tool = SKIP locally, FAIL in CI (`PACKAGING_LINT_STRICT=1`). Say which steps skipped.
- Iterating → run only what change touches: `cd build && ctest -R 'tst_qmlload\.tabs'`. Before every push: full `make check`. CI runs everything (docs-only PR: `packaging-lint` only).
- Local failure → rerun on untouched `main` before calling it environment.
- Gates catch device-only breakage (Qt 5.6 QML on Qt 5.15 host, Harbour rules, `Sailfish.WebView` placement, delegate roles); reasons in BUILDING.md, SCOPE.md §6. Gate red → fix code, never loosen gate or add waiver unasked.
- Tests wait on signals or conditions (`QTRY_*`, `QSignalSpy::wait`), not fixed sleeps or wall-clock asserts (those flake on loaded runners).
- New dependency: Harbour allowed list only (`ci/harbour/`), must earn its keep. Row in `docs/HARBOUR.md` saying why, same PR. Same for any new Sailjail permission.

## UI and words

- Silica stock components, built like existing pages. Jolla's Browser has same feature → match it. Firefox for Android = reference for features and words.
- "Design first" / "no PR yet" → mockup on canvas, wait for approval, then code.
- Every `qsTr()` string: TRANSLATING.md "Writing English". Changed strings → `make translations`, fill all catalogs (TRANSLATING.md "Filling a catalog").
- QML tests stub Silica, no layout: looks unproven until device test. Say so.
- Phone = battery: no always-running animations, timers, polling; stop work when app is in background.

## Comments and docs

- Caveman-full = drop articles, filler, pleasantries, hedging. Fragments OK, short words. Identifiers, paths, code, errors exact. E.g. "Host Qt 5.15 accepts what 5.6 rejects: static QML tests catch it."
- Code comments, `SCOPE.md`, `docs/*.md`, this file: caveman-full. Comments say why, not what (SCOPE.md §6).
- `README.md`, release notes, forum posts: plain prose for users, as short and to the point as possible.
- User-visible change → entry in `docs/CHANGELOG.md` under `## [Unreleased]` (Added / Changed / Fixed / Removed), worded like entries already there, same PR.
- Doc whose subject changed → update it in same PR (SCOPE.md §7).
- No plans, research notes, scratch files in repo.

## Git and PRs

- One branch, one PR per task. Stacked PR only when asked.
- Unrelated finding → tell user; separate PR or issue, not this PR.
- Commit and PR titles: Conventional Commits, plain language, ≤ 72 chars, say outcome: `feat(history): group pages by date`. Types `feat` `fix` `perf` `refactor` `test` `docs` `ci` `build` `chore`; scope = app area or subsystem (`tabs`, `history`, `downloads`, `reader`, `settings`, `cover`, `rpm`, …). Issue number in parentheses when there is one. Body says why, plain prose; brief, to the point.
- PR body: the problem in a sentence or two, then how you fixed it. End with the model and harness that did the work.
- Final diff: no debug logging, no `TEMPORARY` workflow steps, no leftovers.
- Evidence shown in PR body, not just described:
  - UI → before/after device screenshots.
  - Transitions, motion, feel, lag → before/after GIF (preferred: plays inline; mp4 by URL shows as link only) or mp4. GIF from harness mp4 via `ffmpeg`.
  - Performance → before/after chart (image), numbers in table beside it. Frame timing from separate `frames` run: recording slows compositor.
  - Non-visual proof (tests, mutations, logs) → text.
- Evidence on GitHub, never in tree: harness results branch files embedded by URL; made ones (GIFs, charts) pushed to orphan branch `evidence/pr-N`, deleted after merge.
- Flaky test → find and fix cause. Never add retries (`make test` runs none by design: pass on second try = defect). Never re-run CI until green and call it done. Never skip test or loosen assert. One re-run OK only for infra failure before any test ran (runner lost, apt mirror timeout); say so.
- Merge only on user's explicit go. Merge commit, not squash/rebase.

## Releases

- Only on user's go. Latest `main` commit green first, Sonar read too: check main, not just PR.
- Strict semver: nothing breaking → minor.
- Steps: `docs/RELEASING.md`. Harbour upload stays with user.

## Subagents

- Delegate only with tight, written scope.
- Models: only ones on cost/intelligence Pareto frontier ([Artificial Analysis](https://artificialanalysis.ai/), [Slopalytics](https://slopalytics.com/)). Dominated model = don't use. Cheapest frontier model that does the job well. If user names model for task, use that.
- Cheap-model output gets strong-model review: cheap models dropped facts, inverted conditions.

## Reporting

- Replies to user: caveman-full (see Comments and docs).
- Lead with result. Say what was verified, what wasn't, what is guess.
- Non-green step → name it and why. Never hide.
- Forum bug report → search GitHub issues first, link match.
