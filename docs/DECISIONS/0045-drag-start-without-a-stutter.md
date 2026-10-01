# 0045 — The drag up from the bar pays for nothing in its first frame

## Context
Issue #27: dragging the bar upwards to the tab grid stuttered at the very start, every
time. Four things happened at once in the frame the drag was caught, or in the frames
right after it:

* **The deck leapt.** The distance the bar reported was measured from where the finger
  went down, and the drag was only caught once that distance passed
  `Theme.startDragDistance`. The first `dragMoved` therefore moved the deck by the whole
  of that slack in one frame, after the finger had already been moving with nothing
  following it.
* **The grid was drawn for the first time.** `TabsView` is invisible while the page
  covers it, and turned visible on the first frame the deck moved. That frame bound, and
  so uploaded to the GPU, every preview the grid shows -- each a picture half the screen
  in each direction -- with the cells' text and the rows' glass.
* **The picture of the tab being left was taken.** `onDragStarted` grabbed the view:
  a render pass and a read back from the GPU in the next frame, and then, in the grab's
  callback on the GUI thread, a PNG encode of that picture, the old file removed, the
  row and the database updated and the grid's cell decoding the new picture.
* **A slim bar was made whole.** `barCompact` was false while the deck was dragged, so
  on a page scrolled down -- the bar slim, as it is most of the time a page is read --
  the bar grew under the finger for the 200 ms of its animation, and when that ended
  the engine's view was resized and its page laid out again, mid-drag. The same
  happened in reverse as the deck sprang back.

## Decision
* `BarGesture` keeps where the drag was **caught** and measures from there, as Qt's own
  `Flickable` does with its drag start offset: the first `dragMoved` is 0, and from then
  on the deck moves exactly as far as the finger. The threshold that commits is measured
  from the same place, so it is what the deck has visibly travelled.
* A press on the bar or its reach raises **`dragArmed()`** at once, and
  `dragDisarmed()` when it ends without a drag or turns out to be the page's. Armed, the
  browsing page takes the picture of the tab being left and **primes** the deck: the
  grid is visible, out of sight below the page, so its first frame is drawn while the
  finger is still. `settle()` and `unprime()` put it away again. A tap on the bar costs
  one grab; the grab is half size, and a picture taken on a tap is simply a fresher one.
* The grab's callback hands **`result.image`** to `TabModel.storeThumbnail()`, which
  writes it on `ThumbnailWriter`'s one worker thread and reports the path back through
  `updateThumbnail()` when the file is complete. A write overtaken by a newer one for
  the same tab, or finished after its tab closed or went back to the start page, is
  removed rather than shown. The GUI thread never encodes a picture.
* `barCompact` no longer depends on `dragging`: the bar keeps the height it has, and
  the engine's view is not resized because the deck is moving.

## Consequences
The device checklist (`docs/TESTING.md`) has the drag start, slim and whole, under a
finger. The host suite checks each of the four: `tst_qmlload::barDragStartsWithoutAStutter`
drags a real finger a pixel at a time and asserts the grid is drawn and the grab taken on
the press, and that the deck neither moves before the drag is caught nor leaps as it is;
`barStaysSlimWhileDragged` keeps the slim bar and the view's height through a drag;
`thumbnailCapturedOnLoad` asserts the picture is never saved from QML; and
`tst_tabmodel::thumbnailsAreWrittenOffTheGuiThread` and `staleThumbnailWritesAreDiscarded`
cover the writer. Writing the tab's row to the database still happens on the GUI
thread when the picture arrives; it is one small UPDATE and was left as it is.
