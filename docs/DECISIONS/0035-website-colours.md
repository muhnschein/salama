# 0035 — Website colours: pages are dark or light as the ambience is, or as chosen

## Context
Pages can draw themselves for a dark screen or a light one: a style sheet's
`prefers-color-scheme` media query reads which the system uses, and more sites answer it
every year. The engine takes the answer from its widget toolkit, and on the phone that
says nothing of the ambience, so a page is light over a dark ambience that the rest of the
browser follows. Firefox offers the choice as *Website appearance* — Automatic, Light,
Dark — under Language and Appearance; sailfish-browser has no setting for it.

## Decision
**Settings > Website colours** (`Settings.websiteColors`), a Silica `ComboBox` on the main
page under Appearance, beside the screen cutout's switch — one choice of three needs no
page of its own (0028). *Automatic*, the default, is dark on a dark ambience and light on
a light one, as `Reader.isDarkAmbience` tells them apart for the reader view (0024);
*Light* and *Dark* hold whatever the ambience. Stored as 0, 1 and 2, Firefox's order.

**The engine is told through its preferences.** Gecko reads a look-and-feel value from a
preference of its name before it asks the toolkit (`widget/nsXPLookAndFeel.cpp`), and
`ui.systemUsesDarkTheme` is the one `prefers-color-scheme` follows: 1 dark, 0 light. It is
set with `WebEngineSettings.setPreference`, the one way to the engine's preferences Harbour
leaves, as tracking protection's are (0023); the words are `EngineMessages`'
(`websiteColorPreferences()`), so QML carries none. It is given as the browsing page is
made, again when the choice changes, and for Automatic again when the ambience turns from
dark to light or back — never twice for the same answer.

**What the settings ask of the engine has a component of its own**,
`components/EnginePreferences.qml`, made by the browsing page as it makes its
`NotificationCenter`: tracking protection moved there with the website colours, which took
the browsing page from the 600 lines 0010 allows to 585. It imports `Sailfish.WebEngine`,
and `tests/tst_qmlstatic.cpp` allows it there.

## Consequences
A page loaded before a change follows it as the engine re-evaluates its media queries,
without a reload; a page that decides its colours once, in a script, as it loads, keeps
them until it is reloaded. The engine's own form controls and scroll bars follow the same
value. Whether the preference reaches the pages, and whether the engine's own following of
the ambience, where a release has one, agrees with this one, are device checks
(`docs/TESTING.md`). `tst_enginemessages` checks the preference for each choice and
ambience; `tst_settings` the setting; `tst_qmlload::settingsPage` that each choice reaches
the engine through the browsing page, and `rootWindowLoads` that the start does.
