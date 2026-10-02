# 0047 — HTTPS-Only Mode, DNS over HTTPS and Global Privacy Control are Firefox's

## Context
Firefox dropped Do not track in version 135 for Global Privacy Control, and offers
HTTPS-Only Mode, DNS over HTTPS and Safe Browsing besides. SCOPE.md §3 lets this browser
have what the engine does through the preferences it takes; the four were checked against
Gecko ESR 115 and ESR 153 and against sailfishos/gecko-dev 115.39's patches.

- **GPC** is two preferences: `privacy.globalprivacycontrol.enabled` and
  `.functionality.enabled`, both read by `nsHttpChannel::SetGlobalPrivacyControl` for
  `Sec-GPC: 1` and by `Navigator::GlobalPrivacyControl`. The second is off in Gecko's
  defaults on both engines and on in Firefox's own.
- **DNS over HTTPS** is necko's TRR: `network.trr.mode`, `.uri`, `.excluded-domains`, read
  live. Firefox's *Default Protection* is Mozilla's rollout and heuristics, in Firefox's
  front end (`browser/components/doh`), which the embedding does not have.
- **HTTPS-Only** is `dom.security.https_only_mode`; **HTTPS-First**, trying HTTPS and
  falling back, `dom.security.https_first`: off in ESR 115's defaults, on from ESR 140,
  and in Firefox not a setting but what HTTPS-Only Mode off means. A page with no HTTPS
  under HTTPS-Only is toolkit's `about:httpsonlyerror`. Its actor is registered only since
  sailfishos/gecko-dev patch 0076 (May 2026); its Continue is the engine's own
  `document.reloadWithHttpsOnlyException()`, and on ESR 115 its Go Back imports
  `resource:///modules/HomePage.jsm`, a Firefox module the embedding does not ship.
- **Safe Browsing** needs the front end to call `SafeBrowsing.init()` (0023), a Google key
  built into the engine and Firefox's blocked page. None of it is the application's to give.

## Decision
Settings > Privacy takes Firefox for Android's order and words, each translated as Mozilla
translates it: **HTTPS-Only Mode** and **DNS over HTTPS**, each a page, then tracking
protection, then **Tell websites not to share & sell data** with desktop Firefox's
"Global Privacy Control (GPC)" under it, in Do not track's place. `PrivacySettings` keeps
the switches, `DohSettings` DNS over HTTPS; `EngineMessages` says what each asks of the
engine, and `EnginePreferences.qml` gives it on start and on every change, as 0023.

- **GPC** writes both of its preferences, and `privacy.donottrackheader.enabled` false for
  good. A file that had Do not track on starts GPC on, and the old key goes.
- **HTTPS-Only Mode** is Firefox for Android's switch alone: it asks about private tabs,
  and there are none (0019). HTTPS-First is written on whatever the switch says, as Firefox
  has it, and the page says so in desktop Firefox's line, "Salama may still upgrade some
  connections".
- **DNS over HTTPS**: Increased Protection (mode 2), Max Protection (3) and Off (5, which
  nothing else turns on), GeckoView's mapping. No Default: nothing here would decide. The
  line under each level is one Firefox for Android's own pages say of it that the engine
  keeps; Max's promised warning before falling back is the front end's, so it is not
  promised. The provider is Firefox for Android's: Cloudflare (default), NextDNS, or
  Custom, an `https://` address with a host, checked as `DohUrlValidator` does. The
  exceptions are domains, with their subdomains: an address typed in is taken for its host.
  The address and the exceptions are given before the mode.
- **Safe Browsing** is not offered.

## Consequences
On ESR 115 HTTPS-First is new, as it was for Firefox 136's users: a site without HTTPS
loads a moment later. HTTPS-Only Mode depends on the engine's page for a site without
HTTPS, which is the engine's to fix if it breaks; Settings promises only the switch. Max
Protection on a network that needs a login page before anything resolves stops at it until
it is turned down. The icons, `icon-m-keys` and `icon-m-browser`, are sailfish-browser's,
so they are in the theme; whether they read well is a device check (`TESTING.md`).
