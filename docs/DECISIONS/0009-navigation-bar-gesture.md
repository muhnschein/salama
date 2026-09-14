# 0009 — The navigation bar carries the address and the tab gesture

## Context
The design puts back, the address, reload/stop and the menu in one bar along the bottom,
drops the forward button, and reaches the tab grid by dragging that bar upwards rather
than by tapping a button.

## Decision
`components/NavigationBar.qml` holds all four controls. The address is a `Label` until
tapped and a `TextField` in the same place after, so editing needs no second screen.
Forward navigation is dropped from the bar; the gesture replaces the tabs button.

Silica's `PushUpMenu` attaches to a `Flickable`, and the `WebView` is a `QuickMozView`
that scrolls itself, so a stock pulley cannot be used on the browsing page. The bar
handles the drag itself, and **one `MouseArea` covering the whole bar owns every press**:
the icons are icons, the region under the press decides what a tap means, and the same
region drives the pressed highlight. The threshold lives in `isPullUp()` and the regions
in `regionAt()`, so both can be checked from the load tests, which have no window to send
real presses to. The tab grid is an ordinary `SilicaGridView` and uses a real
`PullDownMenu`, whose last item returns to the tab its header names.

The first attempt put that `MouseArea` *behind* the controls, so presses on a button
would reach the button. On device no drag was possible at all: back, the address, reload
and the menu tile the bar edge to edge, so no press ever reached the handler. Letting
presses through instead -- `propagateComposedEvents` -- fails the other way: a handler
that does not accept the press never sees the movement after it. One surface on top, with
the tap dispatched by region, is the arrangement that can do both.

## Consequences
The gesture starts on the bar, not on the page, which keeps it clear of the engine's own
scrolling. Press feedback is drawn from `pressedRegion` rather than by the controls
themselves, because they no longer receive the press. While the address is being edited
the handler stands down, so the field keeps its own taps for the caret, and the bar
cannot be dragged until editing ends.

The menu carries a `Tabs` entry as well. The grid must be reachable when the gesture is
awkward or, as on the first device build, not working at all; a browser whose tabs can
only be reached by a gesture has one way in and no fallback.

Forward navigation is unreachable until it is given a home, which is a Phase 2 question,
not an oversight.
