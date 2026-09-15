# 0012 — The engine's view keeps its size, and pages are laid out larger

## Context
Two things the engine does on its own were wrong for this application on device: page
content was laid out small enough to squint at, and the page was visibly stretched for
as long as the virtual keyboard's open and close animations lasted, snapping back only
when they finished.

## Decision
**Zoom.** `WebEngineSettings.pixelRatio` is set to `Theme.pixelRatio * 1.75`, rounded to
a half the way the platform rounds its own. Sailfish starts the engine at
`1.5 * Theme.pixelRatio` (`sailfish-components-webview/lib/webenginesettings.cpp`), which
lays a page out about 410 css pixels wide on a 1080 wide screen; 1.75 gives 360, the width
a phone-shaped layout is usually written for, and larger text with it. Going further —
2.0 — would take the layout below 320 css pixels, which is narrower than anything sites
are built for.

`pageZoom()` computes it and `engineZoom()` reads back what the engine has, so the load
tests can compare the two. An expression evaluated against the page from outside cannot
see the `Sailfish.WebEngine` import: the context a QML object hands out is the one it was
*created* in, not its own.

**Size.** The deck's layers are `fullHeight` tall — the tallest the page has ever been —
rather than the page's current height. Silica shrinks a page while the keyboard is up, and
resizing the engine's view is a reflow: doing it in the middle of the keyboard's animation
is what left the content stretched until the animation finished. The layers now keep their
size and let the keyboard cover them.

The navigation bar is the exception: its `y` follows the page's *current* height, so it
comes up with the keyboard. The field it carries is the reason the keyboard is open, and a
bar that stayed at the foot of the screen would be typed at from behind it.

## Consequences
`fullHeight` only ever grows, which is right for a portrait-locked application where the
page is full-screen except when the keyboard takes part of it. A device or an orientation
that made the page permanently shorter would leave the layers too tall until a restart;
orientation is locked (SCOPE.md §4), and this is the note to come back to if it is
unlocked.

Zoom is one number in one place, not a setting. A person who wants a different one has no
way to say so yet, and that is a Phase 2 question — the page's own pinch zoom still works
in the meantime.
