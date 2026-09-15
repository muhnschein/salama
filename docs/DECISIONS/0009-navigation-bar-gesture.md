# 0009 — The navigation bar carries the address and the tab gesture

## Context
The design puts back, the address, reload/stop and the menu in one bar along the bottom,
drops the forward button, and reaches the tab grid by dragging that bar upwards rather
than by tapping a button.

## Decision
`components/NavigationBar.qml` holds **the address and the menu**, and nothing else. The
address is a `Label` until tapped and a `TextField` in the same place after, so editing
needs no second screen. Forward navigation is dropped from the bar; the gesture replaces
the tabs button.

Back and reload were on it too, and are not any more: the width of two icons is width the
address did not have, and on a phone the address is what the bar is for. They are
`backItem` and `reloadItem` in the menu, acting on the page through `BrowserPage.goBack()`
and `BrowserPage.reloadOrStop()`. The cost is honest and worth naming: back is two taps
now. It is the price of a field that uses the bar, and if it turns out to be the wrong
trade the smaller one is to bring back only back and leave reload in the menu.

The address is centred on the **screen**, not in the room left beside the menu icon:
`centredWidth` is twice the smaller of the two gaps either side of the centre, so a
centred row can never reach the menu. The field spans the whole of that room, at the size
the host is drawn at, so the text does not jump when a label becomes a field.

Silica's `PushUpMenu` attaches to a `Flickable`, and the `WebView` is a `QuickMozView`
that scrolls itself, so a stock pulley cannot be used on the browsing page. The bar
handles the drag itself, and **one `MouseArea` covering the whole bar owns every press**:
the icons are icons, the region under the press decides what a tap means, and the same
region drives the pressed highlight. The regions live in `regionAt()`, so they can be
checked from the load tests, which have no window to send real presses to.

The bar reports the drag as a **distance**, not as a finished gesture: `dragStarted`,
`dragMoved(distance)`, `dragFinished(distance)`. What that distance moves, and the
threshold that commits it, belong to the page (`0010-tab-grid-deck.md`).

That distance is measured in the **window's** coordinates, through `mapToItem(null, …)`,
and not in the bar's own. The bar rides on the deck: while the deck follows the finger the
bar moves up under it, so a distance measured against the bar shrinks as the deck rises,
which drops the deck back, which grows the distance again. On device that was the whole
screen jumping up and down for as long as the finger was held.

The handler also **reaches above the bar**, by three quarters of
`Theme.itemSizeExtraSmall`. The drag has to
start somewhere the system's bottom-edge swipe has not already taken, and the bar alone
lies in that strip. A tap in the reach does nothing — the page does not get it either,
which is the price of the reach and the reason it is only a strip. The pulley indicator
sits along the bar's **top** edge for the same reason: an indicator at the bottom invites a
thumb to start the drag in exactly the place lipstick is watching.

The first attempt put that `MouseArea` *behind* the controls, so presses on a button
would reach the button. On device no drag was possible at all: back, the address, reload
and the menu tile the bar edge to edge, so no press ever reached the handler. Letting
presses through instead -- `propagateComposedEvents` -- fails the other way: a handler
that does not accept the press never sees the movement after it. One surface on top, with
the tap dispatched by region, is the arrangement that can do both.

## Consequences
The gesture starts on the bar, not on the page, which keeps it clear of the engine's own
scrolling. That also puts it in the strip along the bottom of the screen that the system
watches for its own edge swipe, and on the first device build most upward drags opened
the app grid instead. The bar is `Theme.itemSizeLarge` tall for that reason: every bit
of height is height the drag can start in above what lipstick takes first. It is also
translucent and lies over the page rather than above it, so the height costs the page
nothing. Lying over the page would hide its last rows -- a button in a page footer would
be unreachable -- so **the bar gets out of the way**: `QuickMozView` runs a chrome
gesture of its own, dropping `chrome` when a page is scrolled down by more than
`chromeGestureThreshold` and asking for it back on the way up, and the bar's `y` follows
it off the bottom of the page and back. That is what sailfish-browser's own toolbar does,
and it is the only mechanism that reaches the engine: `RawWebView::setFooterMargin`, the
obvious candidate, forwards to `setDynamicToolbarHeight(m_vkbMargin != 0 ? margin : 0)`
and so does nothing at all unless the virtual keyboard is already up. It is still bound,
because that case is real, but it was not the answer. The threshold is set to the bar's
height, since the engine's own default is zero -- which drops the chrome on the first
pixel of every drag.

Both are set through `Binding` rather than as properties of their own, because an engine
build without one should cost a warning in the log, not a page that fails to load. The bar
also carries a pulley indicator along its bottom edge: it behaves like a pulley, so it
says so. A drag that lipstick takes mid-gesture arrives here as `onCanceled`, which
finishes at zero so the page springs back instead of hanging. Press feedback is drawn from `pressedRegion` rather than by the controls
themselves, because they no longer receive the press. While the address is being edited
the handler stands down, so the field keeps its own taps for the caret, and the bar
cannot be dragged until editing ends.

The menu carries a `Tabs` entry as well. The grid must be reachable when the gesture is
awkward or, as on the first device build, not working at all; a browser whose tabs can
only be reached by a gesture has one way in and no fallback.

Forward navigation is unreachable until it is given a home, which is a Phase 2 question,
not an oversight.
