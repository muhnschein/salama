# 0044 — Settings take sailfish-browser's rows and words

## Context
sailfish-browser has settings this browser had not — Do not track, Enable JavaScript,
Fixed toolbar — and explains the ones both have better: its *Preferred color scheme* says
under it what it is for. The user asked for both, and for a row never to carry an icon and
a switch at once.

## Decision
- **Appearance:** Website colours is *Preferred color scheme*, "The website style to use
  when available", its Automatic called *Match ambience*; the stored values are unchanged
  (0035). The notch guard is 0043's. **Fixed toolbar**, "Always show the bottom toolbar":
  while on, the engine's chrome gesture no longer slims the bar to its handle and host
  (0009); the gesture itself stays on, so nothing else changes.
- **Privacy:** **Do not track**, "Tell sites that I do not want to be tracked", off by
  default, and **Enable JavaScript**, on by default, whose line says "Allowed
  (recommended)" or "Blocked, some sites may not work correctly". Both are the preferences
  sailfish-browser's `WebEngineSettings` properties write — `privacy.donottrackheader.enabled`
  and `javascript.enabled` — given through `setPreference` with the others
  (`EngineMessages::contentPreferences`, `EnginePreferences.qml`), so the values live in
  `PrivacySettings` with the rest of this browser's settings.
- **A switch is not an icon.** `SettingsSwitch` drops its icon; the switch's light is
  centred on the column of icons, with sailfish-browser's arithmetic
  (`_textSwitchIconCenter`).
- **Tracking protection** ends with two sentences on what it cannot promise: the engine on
  the phone has only part of Firefox's protection (0023), whichever engine version it is.
- **Stop**, while a page loads, is a plain cross, `icon-m-reset`, as sailfish-browser's
  stop button is; the cross on a disc reads as clearing a field.

## Consequences
Do not track is a request sites may ignore, and says so by its name. JavaScript off breaks
much of the web; the line under the switch says that as it is switched.
