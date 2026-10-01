# 0043 — The notch guard has sailfish-browser's three modes

## Context
0013 kept every page below the screen's cutout behind a switch, because the two facts
sailfish-browser decides with — whether a page asked for the cutout with
`viewport-fit=cover`, and whether it uses the safe-area insets it is then given — arrive on
its own web page item and not on the `WebView` Harbour allows. The user asked for
sailfish-browser's control instead: *Notch guard*, Automatic, Forced or Disabled, with its
description.

## Decision
`Settings.notchGuard`, a combo box under Appearance with sailfish-browser's label, words,
description and icon:

* **Automatic** (default) keeps a page below the cutout unless its viewport meta tag says
  `viewport-fit=cover`; such a page gets the whole screen, and the platform `WebView`'s own
  safe area tells it where the cutout is. The tag is asked of the page after each load, as
  its theme colour is (`EngineMessages::viewportScript`, `coversCutout`), and forgotten as
  the next load starts.
* **Forced** keeps every page below it: 0013's guard as it was.
* **Disabled** keeps none: 0013's switch off.

The tab grid's head row, the address bar's pane and the tutorial keep out of the cutout
unless the guard is disabled (`Settings.cutoutGuard`).

A settings file from before keeps what it said: the old switch, if it was ever stored,
becomes Forced for on and Disabled for off, and the key is removed.

## Consequences
Automatic is coarser than sailfish-browser's, which also wants the page to *use* the safe
area before giving it the cutout; nothing on the `WebView` says whether it does. A page that
asks for `cover` and then ignores the inset has its first line under the cutout — the page's
own request, which Forced overrides.
