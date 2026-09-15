# 0010 — The tab grid is a deck below the page, not a page of its own

## Context
The first build pushed `TabsPage.qml` onto the page stack when the navigation bar was
dragged upwards. On device that reads wrong in two ways at once: the transition is
Silica's page transition, so an upward drag produces sideways movement, and nothing at
all moves until the drag passes its threshold — which, in the strip the system watches
for its own bottom-edge swipe, is indistinguishable from the gesture having been taken
away.

## Decision
The browsing page and the tab grid are one **deck two screens tall**: browsing above,
`components/TabsView.qml` directly below it. `BrowserPage.tabsOffset` is how far the
deck has been raised, from 0 (browsing) to the page height (the grid), and the deck's
`y` is its negative. Nothing is pushed onto the page stack for the grid, so there is no
sideways transition, no second page, and no back gesture to come back through.

The offset comes from one of two places, never both:

* `dragging` is true while a finger is on it, and the offset is whatever the finger
  says: the bar's `dragMoved(distance)` going up, the grid's `pulled(distance)` coming
  back down. The deck tracks the finger from the first `Theme.startDragDistance` of
  movement, so the gesture is visibly caught long before it commits.
* otherwise the offset is a binding on `tabsOpen`, and a `Behavior` carries it there.

`tabsOpen` is the settled answer and flips the moment a gesture commits; `tabsOffset` is
only the picture. Tests assert the first and never have to wait for an animation. The
spring is switched on and off from `beginDrag()` and `settle()` rather than by a binding
on `dragging`: the drag ends by changing what `tabsOffset` is bound to, and two bindings
on the same property are not ordered against each other, so the spring has to be back on
before the target moves rather than in the same breath.

Coming back is the grid's **own overscroll**, not a second handler on top of it. Dragged
past its top the view reports the distance and sets its own `y` to the negative of it,
which cancels the shift the flickable would draw: the content then stays exactly where
the finger put it while the deck behind it slides down and brings the page back. A
handler laid over the grid would have to choose between the pull and the grid's own
scrolling at press time, before the direction is known; nesting flickables has the same
problem, because in Qt Quick the inner one keeps the drag at its bounds instead of
handing it on. The grid is also `Flickable.VerticalFlick` rather than the default
automatic direction: with a handful of tabs the content fits the screen, and an
automatic flickable refuses to be dragged at all — exactly the case where the way back
would go missing.

Tapping a preview does the same thing as the pull, with the tab it names.

A cell can also be **carried** to another place in the grid. The gesture is a sideways
drag, because the grid only ever flicks up and down: across the cell is the one movement
nothing else is waiting for, and it needs no press-and-hold to disambiguate. Once a cell
is held the delegate sets `preventStealing`, so the grid cannot take the drag back, and
what moves is the cell's *contents*, not the cell: the view owns where cells are, and
after `TabModel.moveTab()` the cell underneath has already moved to meet them. Half of
`Theme.startDragDistance` is enough to pick a cell up, rather than the whole of it: nothing
else is waiting for that movement, so the cell can come up as soon as the finger goes
sideways. Only the
displaced cells are animated; the carried one is under a finger and must not be animated
away from it.

A cell that has been carried must not also *open* when the finger lifts. `MouseArea`
raises `released` before `clicked`, so the flag the release resets cannot be the one the
click reads: `held` ends the carry, and a second flag, `carried`, lives from the moment
the cell is picked up until the next press and is what `releaseTap()` asks.

The grid carries two rows of its own, both drawn over the cells in the same glass as the
navigation bar rather than scrolling among them, each with a spacer of the same height in
the view's header and footer so that no cell is stranded under either:

* along the **head**, what the grid holds — "*n* tabs". It replaced a page header that
  named the active tab, which said what the page behind the grid already says. Its real
  work is the row of cells below it: without it the first row, and the close button in its
  corner, sat under the device's own screen cutout.
* along the **foot**, the one control the grid offers: new tab.

A preview is drawn as wide as its cell and anchored to the cell's **top**, at the
picture's own aspect ratio, rather than with `PreserveAspectCrop`. The picture is of a
screen — tall — and the cell is not; cropping to fill centres it, so every preview showed
the middle of a page whatever the reader had been looking at. Anchored at the top, what
shows is the top of what was last on the screen.

The grid's `PullDownMenu` is gone. It was the only pulley in the application, it sat
inside a view that now owns dragging past its own top for the way back, and two
meanings for one drag is one too many. What it carried went elsewhere: "Go to tab" is
the pull and the tap, "New tab" is the button in the grid's header, "New private tab" is
in the menu, and "Close all tabs" is in Settings next to the other clearing actions.

## Consequences
The grid is instantiated with the page rather than on demand, so the delegates exist
before they are first shown; it is `visible` only while the deck is raised, so the
engine has the screen to itself the rest of the time. The previews are captured when a
drag starts, which is before the first pixel of the grid can be seen — and a drag that
springs back has captured one for nothing, which costs a grab and a file.

That grab lands at the worst moment for it: a read back from the GPU and a PNG encode,
on the first frame of a gesture. On device the transition was visibly rough for it, so
the grab is taken at half the screen's width and height. The grid never draws a preview
wider than half the screen, so nothing is lost, and there is a quarter as much to read
back and encode. If it is still rough, the next thing to try is capturing after the deck
has settled rather than before it moves — at the cost of a preview that is one gesture
out of date while the grid comes up.

The deck's two layers sit outside the window when the deck is at either end, and the
window is what clips them; nothing is set to `clip`, which the engine's own composited
surface would not have honoured anyway.

While the grid is open the browsing page is still the page on the stack. Anything that
reads `pageStack.currentPage` sees `browserPage` either way, and the menu's `Tabs` entry
pops back to it and calls `showTabs()` on it rather than pushing a page of its own.
