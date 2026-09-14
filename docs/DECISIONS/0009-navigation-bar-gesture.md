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
that scrolls itself, so a stock pulley cannot be used on the browsing page. A `MouseArea`
behind the controls tracks the drag instead: presses that land on a button reach the
button, and the threshold lives in `isPullUp()` so it can be checked from the load tests,
which have no window to send real presses to. The tab grid is an ordinary `SilicaGridView`
and uses a real `PullDownMenu`, whose last item returns to the tab its header names.

## Consequences
The gesture starts on the bar, not on the page, which keeps it clear of the engine's own
scrolling. Dragging from the address area is taken by the tap handler; the bar's other
space starts the drag. Forward navigation is unreachable until it is given a home, which
is a Phase 2 question, not an oversight.
