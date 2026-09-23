# 0023 — Tracking protection is Firefox's, set through the engine's preferences

## Context
Firefox's Enhanced Tracking Protection lives in Gecko, and so does this browser: ESR 115
on Sailfish OS 5.x today, ESR 153 once the stack in
[sailfishos/sailfish-browser#1143](https://github.com/sailfishos/sailfish-browser/pull/1143)
ships. SCOPE.md §3 allows no content blocking beyond what `WebEngineSettings` exposes, and
§9 asked whether it exposes tracking protection. It does, as preferences:
`WebEngineSettings.setPreference(name, value)` (qtmozembed `QMozEngineSettings`) is
invokable from `Sailfish.WebEngine`, and it queues what it is given until the engine is up.

Preferences are half of ETP. Blocking needs lists, and the URL classifier gets them only
once the front end calls `SafeBrowsing.init()` — Firefox's BrowserGlue does, GeckoView's
startup does, and nothing in the embedding does (gecko-dev `embedding/embedlite`,
embedlite-components, sailfish-components-webview, on ESR 115 and ESR 153 alike). Its
tables stay empty, and `privacy.trackingprotection.enabled` blocks nothing. ESR 153 adds a
second path, the content classifier (`toolkit/components/content-classifier`): adblock
lists from Remote Settings, started by its own preferences on the first request it is
asked about. It needs nobody's `init()`.

## Decision
Settings > Privacy > Tracking protection: **Off, Standard (default), Strict** — Firefox's
categories, Off in place of Custom. `EngineMessages::trackingProtectionPreferences()` says
what each asks of the engine; `BrowserPage` writes it on start and on every change. Every
level writes the same twelve preferences, so none leaves anything behind in the profile.

* **Standard** is Firefox's default: cookie behaviour 5, Total Cookie Protection; the
  content classifier annotating trackers, which is what refuses them their cookies, and
  blocking fingerprinters and cryptominers, with both exception lists.
* **Strict** is `browser.contentblocking.features.strict`: every tracker list blocked, the
  level-2 list counted, fingerprinting protection, query stripping, referrers trimmed on
  top-level navigation, bounce tracking protection, and the major exception list only.
* **Off** is the engine's own defaults.

The URL classifier's switches (`privacy.trackingprotection.enabled` and the rest) are left
alone: with no lists they cost a lookup per request that cannot match, and a setting
claiming them would claim what the engine does not do.

| | ESR 115 | ESR 153 |
|---|---|---|
| Total Cookie Protection | yes | yes |
| Fingerprinting protection, referrer trimming (Strict) | yes | yes |
| Query stripping (Strict) | list from Remote Settings | list from Remote Settings |
| Trackers annotated and blocked | no | list from Remote Settings |
| Bounce tracking protection (Strict) | no | yes |

A name ESR 115 does not know stays a preference nothing reads. The descriptions under
the setting promise only what both engines do.

## Consequences
On ESR 115, Standard is Total Cookie Protection and little else; the rest arrives with the
engine, with no change here. The embedding reads Remote Settings from the dumps it ships
(search engines, gecko-dev's ESR 153 patch 0045); no dump ships for these lists, and
Firefox 153 turns the content classifier on through experiments rather than by default, so
whether the lists sync from Mozilla's server here — `content-classifier-lists` above all —
is seen on the device (`docs/TESTING.md`). If they do not, nothing is blocked by list and
nothing breaks.

Cookie behaviour 5 gives a site inside another's frame cookies of its own, so a login
made in the one does not carry into the other: Firefox's default since version 103.

Should the embedding ever start `SafeBrowsing`, the URL classifier's switches become worth
a row in the table.
