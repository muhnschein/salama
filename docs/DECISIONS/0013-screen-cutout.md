# 0013 — The display's cutout is kept clear, and that can be turned off

## Context
Recent Sailfish devices have a camera cutout at the top of the screen. tuuli draws to the
whole screen, so the cutout sat over whatever was underneath it: on device it took a bite
out of the tab grid's head row, and it sits over the first line of every page.

## Decision
`Settings.cutoutGuard` — "Avoid the screen cutout" in Settings — is **on by default**. A
notch over the first line of a page is not a design decision, and a browser that has to be
configured before it can be read is a browser that failed at the thing it is for.

What the cutout takes is `Math.max(0, Screen.topCutout.y + Screen.topCutout.height)`: the
whole rectangle Silica reports, not its height alone, because a cutout need not start at
the very top of the screen. That is the same expression sailfish-browser uses
(`apps/shared/WebView.qml`), and it is read through a guard so that a `Screen` without a
cutout to report gives zero rather than an undefined length.

While the guard is on:

* the engine's view starts below the cutout, and the strip it leaves is painted in the
  page's own **theme colour** when it declares one, as sailfish-browser paints it;
* the tab grid's head row is that much taller and puts "*n* tabs" below the cutout, which
  is also what keeps the first row of cells and their close buttons clear of it;
* `safeAreaTop` on the view is bound to **0**. The platform's `Sailfish.WebView` hands the
  engine a safe area computed from the same cutout, so that a page written for one can lay
  itself out around it — but with the view already below the cutout there is nothing left
  for a page to avoid, and a page that did would be avoiding it twice.

Turning it off gives the cutout's height back to the page and puts the grid's head row
where it was.

That colour is asked of the page, with `runJavaScript` and the same shape of script the
favicon uses — a function body that returns, for the reason `0005-favicons.md` gives. sailfish-browser reads it as a property instead — but
from its own `DeclarativeWebPage`, which Gecko sends a message carrying `viewportFit`,
`safeAreaInsetUsage` and `themeColor` together (`apps/qtmozembed/declarativewebpage.cpp`);
none of that reaches the `WebView` Harbour allows. `EngineMessages::themeColor()` decides
whether the answer is a colour at all: CSS writes colours in forms `QColor` does not read
— `rgb()`, and the eight-digit hex whose *last* pair is alpha where Qt's is its first —
and a page can put anything in that attribute. Alpha is dropped, as upstream drops it: a
translucent band would show what is behind it, which is the one thing a band painted in
the page's colour must not do.

## Consequences
This is a coarser instrument than the platform browser's. sailfish-browser reads a policy
from dconf with three values and lets a page under the cutout when it asks for that with
`viewport-fit: cover` *and* actually uses the safe-area insets it is given
(`_policyAllowsCoverViewportFit`). Both of those facts — the page's `viewportFit` and its
`safeAreaInsetUsage` — live on the browser's own web page item, not on the `WebView` that
Harbour allows; tuuli cannot see either, so it cannot tell a page that has thought about
the cutout from one that has not. The guard is therefore on or off for every page, and the
direction it errs in is the safe one: content visible where a page did not ask for
anything, at the cost of the cutout's height for the pages that did.

The cost is real estate — the cutout's height, on every page, all the time — which is why
there is a switch. Landscape is not handled at all, because tuuli is portrait-only; the
day that changes, the rest of upstream's cutout arithmetic (which edge the cutout is on
for each orientation) is what to port.
