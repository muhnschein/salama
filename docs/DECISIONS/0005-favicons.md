# 0005 — Favicons through a page script with `/favicon.ico` fallback

## Context
`WebView` exposes no favicon property (qtmozembed `qmozview_defined_wrapper.h`).
sailfish-browser receives icons through engine messages that are private to it.

## Decision
When a page finishes loading, the view runs `EngineMessages.faviconScript`
(`document.querySelector('link[rel~="icon"]')`) through the public `runJavaScript`
API and resolves the result against the page URL; without a declared icon, or on
error, `scheme://host/favicon.ico` is used. QML `Image` loads the URL; the RPM
requires `qt5-plugin-imageformat-ico`.

A script handed to `runJavaScript` is the **body of a function**, and has to `return`.
The engine builds the function from the string and calls it — `new
content.Function(jsstring)` in embedlite-components `jsscripts/embedhelper.js` — so a
script that merely *evaluates* to something, an immediately-invoked function among them,
hands the callback `undefined` and says nothing at all. This script was written that way
at first, which is why every page quietly fell back to `/favicon.ico`; the same mistake
made the theme colour in `0013-screen-cutout.md` never arrive. Both are one form now, and
the tests assert it.

## Consequences
Icons are fetched by Qt, not by the engine, so a second request per page. Private tabs
never publish their icon. Verified on the device as part of the smoke test.
