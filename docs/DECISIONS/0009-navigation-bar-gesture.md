# 0009 — The navigation bar carries the address and the tab gesture

## Context
The design puts back, the address, reload/stop and the menu in one bar along the bottom,
drops the forward button, and reaches the tab grid by dragging that bar upwards rather
than by tapping a button.

## Decision
`components/NavigationBar.qml` holds **back, the address, reload/stop and the menu**. The
address is a `Label` until tapped and a `TextField` in the same place after, so editing
needs no second screen. Forward navigation is dropped from the bar; the gesture replaces
the tabs button.

Back and reload were moved into the menu for one build, on the grounds that the width of
two icons was width the address did not have. On device that was the wrong trade — back
is the control a browser uses most, and two taps for it is two taps — so they are on the
bar again and gone from the menu. What the address gets instead is the bar **while it is
being edited**: back and reload are not drawn then, the field spans from the edge of the
screen to the menu, and the text inside it is inset by `Theme.paddingMedium` rather than
by Silica's own default of a page margin at each end. A field is worth every pixel of the
bar, but only while there is something to type into it.

Those two insets are set through `Binding` rather than as properties, for the same reason
the engine's are below: assigning to a property a Silica build has not got is a load
error, binding to one is a line in the log.

The address is centred on the **screen**, not in the room left between the controls:
`centredWidth` is twice the smaller of the two gaps either side of the centre, so a
centred row can never reach one of them.

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
lies in that strip. What says the bar can be dragged is drawn along its top edge, inside
that reach: `components/DragHandle.qml` (`0010-tab-grid-deck.md`).

The reach lies over the foot of the page, and at first it kept every press there: a tap
in it did nothing, and the page did not get it either. On device that was the controls
of every player whose foot sat at the foot of the screen — a seek bar, play, full
screen — out of reach, full-screen video worst of all. So the reach **hands the page
everything that is not the drag**. A press there is undecided until the finger moves or
lifts: moved upwards past `Theme.startDragDistance` it is the drag, as before; moved
any other way that far, or lifted without moving that far, it is the page's, and the
browsing page gives it to the engine as the touch it would have had —
`synthTouchBegin`, `synthTouchMove`, `synthTouchEnd`, which qtmozembed's view takes in
its own coordinates, the ones a real touch arrives in — from where it went down. The
view takes focus as a real touch gives it, which also ends editing the address. A
press **held** in the reach is not handed on: the handle is in the reach, and a finger
resting on it before it moves up is the grid's. A long press on a link in that strip is
the price of that.

Detecting players instead — asking the page where its media sits, and shrinking the
reach over one — was the other way. It needs a script run on every scroll, and it
could only ever find the players it knows the shape of; the hand-back covers whatever
is under the finger. Shrinking the reach alone would have given back only as much of a
player as it gave up, and the drag would have lost the room.

The handler is `components/BarGesture.qml`, the bar's own file having grown past what
one responsibility should take.

The bar is **opaque**, and the engine's view ends where the bar begins: `viewArea` is
`fullHeight - barHeight` tall. Both of those replace a translucent bar that lay over the
page and took itself off the screen on the engine's own chrome gesture. That was prettier
and it did mostly work, but "mostly" is the problem: on device the last rows of a page — a
footer, a cookie banner's buttons — kept being unreachable, and a control you can reach
four times out of five is a defect. A bar that never covers anything cannot hide anything.

What the bar does with that gesture instead is **shrink**. Scrolled down, it drops to
`Theme.itemSizeSmall` — a quarter less than `Theme.itemSizeLarge` — takes back, reload and
the menu off itself, and draws the host at `Theme.fontSizeSmall`: the handle and the
address, which is what a bar is for while a page is being read. Scrolled back up, it is
whole again. `chromeGestureThreshold` is bound to a constant rather than to the bar's own
height, because the bar answers that gesture by changing height and a threshold that moved
with it would be chasing itself.

The change of state is **one animation**, 200 ms on the bar's own `height`. Everything
that differs between the two states is drawn from `expansion`, which is 0 at the slim
height and 1 at the whole one: the three controls fade with it, the host's size is
interpolated between `Theme.fontSizeSmall` and `Theme.fontSizeMedium` by it. Nothing
else animates, because nothing else has to.

The slim bar is **opaque**, as the whole one is. It was fully transparent first, with
the page running on behind it, and on device the host was unreadable over a light page:
white text on white. A wash of `Theme.opacityLow` was tried next and was still not
enough to read by. So the bar keeps its colour at either height, and the page **ends
above the slim bar** as it ends above the whole one: nothing is hidden under it, and
the whole of it takes presses, since nothing under it is the page's to take. What the
slim bar gives the page is the quarter of its height it gives up, which is what the
gesture was for.

`viewArea` is sized from the bar, so the engine's view grows into what the bar gives up —
but sized for the **slim** height for as long as the bar is between the two (`resizing`).
A view resized on every frame of that animation is a page relaid out on every frame, which
is the mechanism that deformed pages while the keyboard was animating; sized for the
slimmer height throughout, the view is resized once per change of direction and the bar
covers the difference while it moves. Editing forces the whole bar back, since the field
needs the room, and so does a page starting to load, since a new page starts at the top.

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

That binding is set through `Binding` rather than as a property of its own, because an
engine build without it should cost a warning in the log, not a page that fails to load.
`footerMargin` is not bound at all any more: it reaches the engine only while the virtual
keyboard is up, which is why it could never have been the answer here. A drag that
lipstick takes mid-gesture arrives here as `onCanceled`, which finishes at zero so the
page springs back instead of hanging. Press feedback is drawn from `pressedRegion` rather than by the controls
themselves, because they no longer receive the press. While the address is being edited
the handler stands down, so the field keeps its own taps for the caret, and the bar
cannot be dragged until editing ends.

The menu carries a `Tabs` entry as well. The grid must be reachable when the gesture is
awkward or, as on the first device build, not working at all; a browser whose tabs can
only be reached by a gesture has one way in and no fallback.

Forward navigation is unreachable until it is given a home, which is a Phase 2 question,
not an oversight.
