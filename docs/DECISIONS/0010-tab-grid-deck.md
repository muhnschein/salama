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

A cell can also be **carried** to another place in the grid, and **slid away** to close
its tab. The two share the sideways movement, so a hold tells them apart: a finger held
for **a second** picks the cell up (a timer the delegate owns, since
`MouseArea.pressAndHoldInterval` came with Qt 5.9), and the cell comes up a little so the
hand knows it has it. It was a second and a half until the person testing on device
asked for a second. A finger that moves sideways before then is sliding the cell, to
the left only, because the grid has nothing to the right. Slid past a third of its width
and released, the cell closes its tab; released short of that it slides back. It fades as
it goes, so the finger sees what lifting will do. A first build picked the cell up on
the sideways movement alone, with no hold, and on device that was too easy to do by
accident and left no gesture for closing. Once a cell is held or sliding the delegate sets
`preventStealing`, so the grid cannot take the drag back -- and **not before**, which is
the subject of the next section. What moves is the cell's *contents*, not the cell: the view owns where cells are, and after `GroupTabs.moveTab()`
the cell underneath has already moved to meet them. Only the displaced cells are animated;
the carried one is under a finger and must not be animated away from it.

A cell that has been carried or slid must not also *open* when the finger lifts.
`MouseArea` raises `released` before `clicked`, so the flag the release resets cannot be
the one the click reads: `held` ends the carry, and a second flag, `carried`, lives from
the moment the cell is picked up or slid until the next press and is what `releaseTap()`
asks.

The close button in a cell's corner is a **disc with a cross through it**, drawn by the
cell; the theme icon it replaced is discussed below.

The grid carries two rows of its own, both drawn over the cells rather than scrolling
among them, each with a spacer of the same height in the view's header and footer so that
no cell is stranded under either. Both are **children of the flickable itself** --
declared beside it and handed to it once made, since anything declared inside a view goes
into the content it scrolls -- because a flickable filters the presses of its own children
and nothing else's: beside it, the strip of groups took every press along the head of the
screen, which is exactly where a pull down begins. On the flickable they ride on its `y`,
which moves up as the view is pulled down, so each carries the overscroll as a margin and
stays with the content:

* along the **head**, Silica's own `SearchField`, "Search tabs", across the row with its
  words from the left edge; what it finds is listed over the cells (0015). The head held
  the way to a new tab and a search button that opened a page of its own, and before
  that the strip of tab groups (0015) until the strip went to the foot, and before that
  "*n* tabs", and before that a page header that named the active tab, which said what
  the page behind the grid already says. The row's other work is the row of cells below
  it: without it the first row, and the close button in its corner, sat under the
  device's own screen cutout. The row is `Screen.topCutout.height` taller than it looks
  and puts its controls below that height, which is what Silica's `PullDownMenu` does
  with its top margin; the property is read through a guard, so a `Screen` that does not
  report a cutout gives a row of the ordinary height rather than one that is undefined
  pixels tall.
* along the **foot**, new tab in the left corner, the strip of tab groups, and the way to
  edit them in the right corner. The corners are the same width, so the names are
  centred on the screen. Held rather than tapped, new tab brings up the tabs closed
  lately (0018). The strip was in the head until a tab could be carried onto a group to
  move it there (0015): at the foot it is under the thumb doing the carrying.

Both rows are panes of **Silica's glass**: their tint, `Theme.highlightDimmerColor` at
`Theme.opacityOverlay`, with the ambience's own pattern, `Theme._patternImage`, tiled over
it a pixel to a pixel of the screen and drawn at a tenth, as Silica's glass material draws
its pattern; the keyboard's glass draws this one (`components/GlassTexture.qml`). The tint
alone was a smooth band where Silica's own panes are textured. The material itself is in
`Sailfish.Silica.Background`, which is not on Harbour's import allow-list; the pattern is
the theme's, an underscored `Theme` property as `_lineWidth` is (0015).

A preview is drawn as wide as its cell and anchored to the cell's **top**, at the
picture's own aspect ratio, rather than with `PreserveAspectCrop`. The picture is of a
screen — tall — and the cell is not; cropping to fill centres it, so every preview showed
the middle of a page whatever the reader had been looking at. Anchored at the top, what
shows is the top of what was last on the screen.

The cell is a plain `Item`, not a Silica `BackgroundItem`. That one draws both its press
feedback and its highlight as a wash across the whole cell, edge to edge. The cell draws
the same square wash itself, stopped short of its edges round the picture and the title,
and marks its rounded box with a border as well. The wash was once left out for the border
alone, which on device said nothing at all; it came back with the box's rounded corners,
and after another look on device it is square, as Silica's own is.

The box has rounded corners, and so does the picture: `clip` is rectangular
whatever the shape of the item doing the clipping, so the corners are cut by an
`OpacityMask` from `QtGraphicalEffects` — which is on Harbour's import allow-list, and is
how sailfish-browser rounds its own tab previews
(`apps/browser/qml/pages/components/TabItem.qml`). The mask costs one framebuffer per
visible cell, which is the price of the shape; the radius is `Theme.paddingMedium`, the
same number upstream writes as `12 * Theme.pixelRatio`.

### Every drag up or down is the grid's
The grid is pulled back, and scrolled, from **anywhere on it**: a cell, the gaps between
cells, the head row. The foot row hands its presses to the grid as well, but a pull begun
there has too little screen below it to go past the threshold. The cells do not keep a
press for themselves while a hold is still forming, and that is a rule of Qt's, not a
preference.

The build that introduced the hold did keep it: `preventStealing` was up from the press,
so that a thumb drifting while it held would not hand the grid a scroll before the hold
ran out, with the expectation that past the drift tolerance the grid would "take the drag
from the next move". It never did. A `Flickable` that filters a move while another item
keeps the grab gives the touch up for good -- it forgets the press (`lastPosTime = -1`,
`pressed = false`, in `QQuickFlickable::sendMouseEvent` on Qt 5.6 and `filterMouseEvent`
on 5.15) and ignores every move after it -- and it cannot be handed the touch back. So any drag begun on a
cell went nowhere: the page could only be pulled back from the gaps between cells, and a
grid longer than the screen could not be scrolled from one. Qt 5.6 also runs every
ancestor's filter on every event whatever a nearer one decided, so no item placed between
the cell and the grid can hold the flickable off without the same result.

What that costs is drift. A hold now tolerates `Theme.iconSizeSmall` of it **sideways**,
where only the slide is waiting, and **up and down only as far as the grid's own drag
distance**: past that the grid takes the drag, the cell's handler is cancelled, and the
hold is off -- which is what a press-and-hold does in every Silica list. Keeping the full
tolerance vertically was possible only by having the cell keep the touch and drive the
grid's scrolling and pulling itself, by hand, for every drag that starts on a cell: the
flickable's own physics traded for an imitation, on the gesture the grid is used for
most. The shorter hold is the other half of the answer; a second is less time to drift.

Sideways is the cell's, and it has to be claimed before the grid can take it. A slide
slants as a thumb does, and a slide that drifted down by the grid's drag distance before
it had gone the hold's tolerance across went to the grid, to scroll or to pull. So once
the finger has moved more across than up or down, by three quarters of that drag
distance, the cell keeps the touch: from then on it is a hold or a slide and never the
grid's. The distance is Qt's style hint, `Qt.styleHints.startDragDistance`, since that
is the one the flickable measures by, rather than Silica's `Theme` value.

The gestures are tested under a real finger (`tst_qmlload::gridGesturesUnderAFinger`):
the application is put in a window and pressed on, because whether a drag begun on a
cell reaches the grid is decided inside Qt's event delivery, which raising the gesture's
signals from a test skips entirely.

The grid's `PullDownMenu` is gone. It was the only pulley in the application, it sat
inside a view that now owns dragging past its own top for the way back, and two
meanings for one drag is one too many. What it carried went elsewhere: "Go to tab" is
the pull and the tap, "New tab" is the button in the grid's foot row, and "Close all
tabs" is in Settings next to the other clearing actions.

### What says an edge can be dragged
**Silica draws nothing for this.** Jolla's own source settles that much: `PullDownMenu`
keeps a `menuIndicator` property "for API compatibility" whose only effect is to log that
it is no longer supported, and what the platform offers instead is `PulleyAnimationHint`
— a movement, peeking the menu open by `Theme.itemSizeExtraSmall` over 400 ms and letting
it fall back over another 400 ms.

That movement was built and shipped, and on device it was not wanted: a hint that plays
once, before the hand is anywhere near the screen, is a hint nobody is looking at. So
`components/DragHandle.qml` is drawn instead — a short rounded bar along the navigation
bar's top edge — and it is deliberately more than a decoration: it sits inside the reach
the gesture handler already covers, and it lights up (`active`) while that gesture has the
finger, so the thing you aim at is the thing that responds. This is not what Silica does;
it is what this application needs, and the previous two attempts to guess at a platform
idiom for it were both wrong. The grid's top edge had the same handle for a while, and on
device it read as a second handle to find; it now has a **line across the very top of the
screen**, as thick as the handle, which is how Silica's own pulley menu says it is there —
in the highlight *background* colour, the highlight itself being too loud a line to have
across the top of every grid. The hold tolerates drift: a thumb held down moves, and the
first build wanted it perfectly still. How much, and in which direction, is in the
section above.

The close button on a cell is drawn by the cell (`closeTabMark`, a `PreviewButton`, which
the media controls along the picture's foot are made of too): a disc in the highlight
colour with a cross through it. The theme's `icon-m-clear` carries a disc of its own at
its own transparency, baked into the icon, so the glyph alone was lost on most pages and
a disc drawn behind it was a disc inside a disc. The disc was all but opaque at first,
and on device it was the first thing seen on every cell; it is drawn at
`Theme.opacityHigh` now, and opaque only under a finger.

The picture and the title sit `Theme.paddingMedium` and half a `Theme.paddingSmall` in
from the cell's edges (`inset`), so two cells stand twice that apart. It was a medium
padding, and on device the pictures stood too close together.

### The size of the browsing page
`qml/pages/BrowserPage.qml` is over the 400 lines SCOPE.md §7 allows a QML file, and is
waived here rather than split:

qml-size-waiver: qml/pages/BrowserPage.qml

The rule's other half is one responsibility per file, and the usual answer to a long file
— take a responsibility out of it — is not available to this one. SCOPE.md §5 puts the
`Sailfish.WebView` import in the browsing page **and nowhere else**, so that a release
without the engine package breaks browsing rather than the application; the page is a
`WebViewPage`, and the per-tab `WebView` component with its favicon, thumbnail and engine
bindings is another eighty lines that cannot move out of it. What could be taken out has
been: the bar, the address, the grid, the cell, the handle are all components of their
own. What is left is the engine, the deck it sits in, and the state the two share, which
is one subject. The lint's ceiling of 600 lines still applies, and if this page reaches
it the split to make is the deck's state and gestures, not the engine.

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
reads `pageStack.currentPage` sees `browserPage` either way.
