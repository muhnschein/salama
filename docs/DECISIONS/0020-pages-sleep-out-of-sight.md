# 0020 — Pages sleep out of sight, unless one is making a sound

## Context
With a busy site open and the application put away, the phone stayed busy: the
processor ran as if the page were on the screen. The views went inactive out of sight —
`WebView.active` false, which stops the engine drawing and lets it slow a page's timers
down — but a page's timers, workers, idle callbacks and the scripts they run went on,
and a busy page has plenty of each.

sailfish-browser does not leave its pages to that. Out of sight for a second it calls
`suspendView()` on the page in front (`apps/shared/ResourceController.qml`), and every
page it leaves for another tab is suspended as it is left (`apps/core/webpages.cpp`).
`suspendView()` is a slot of qtmozembed's `QuickMozView`, which the Harbour `WebView`
is: the view goes inactive, the page's inner window is **frozen** — `Freeze()` in
embedlite's `EmbedLiteViewChild::RecvSuspendTimeouts`, which stops its timers, its
workers and its idle callbacks — and the engine stops drawing into its window.
`resumeView()` undoes all three.

What it does not suspend is a page making a sound: a player feeds its stream and starts
its next track from the page's own scripts. Nor does it make that page inactive, which
matters as much. Sailfish's Gecko pauses the media of a **hidden** document — the same
patch that reports playback below, `rpm/0056-Ensure-audio-continues-when-screen-is-locked.-Contri.patch`
in sailfishos/gecko-dev, changed the test from an inactive document to a hidden one —
and an inactive view's document is hidden: `SetIsActive(false)` makes its browsing
context inactive, its window a background one, and its document's visibility hidden.
sailfish-browser keeps its page active out of sight for that reason, and stops it
drawing by other means the Harbour `WebView` does not offer. The browser knows a page is
playing from two observer topics: every media decoder reports its metadata (does the
stream have sound) and each change of its play state on `media-decoder-info`, from that
patch, and a WebRTC call reports its microphone and camera on `webrtc-media-info`.

## Decision
`src/engine/PageActivity` (`PageActivity`) decides when; the browsing page does it.

- `PageActivity` subscribes to nothing itself: it lists the two topics, the browsing
  page hands them to `WebEngine.addObserver()` and passes on every `recvObserve`. It is
  `audible` while a decoder whose stream has sound is playing — one whose metadata was
  never seen counts as sound — or a call is capturing. A muted hero video with no sound
  in its stream does not count, which is the busy page this is about.
- The browsing page tells it when the application leaves the screen and comes back.
  **One second** out of sight and not audible, and it is `asleep`; if sound stops while
  out of sight, **five seconds** after — sailfish-browser's two numbers, the second
  because a second was too little for a player to start its next track over a mobile
  connection. Back on the screen it is awake at once.
- The view in front is `active` until the pages are asleep, not until the application
  leaves the screen: through the second's grace, for as long as it makes a sound, and
  through the five seconds after, so its document stays visible and its media plays.
  It goes inactive as the pages go to sleep.
- Asleep, the browsing page calls `suspend()` on **every loaded page**, not only the
  one in front: the four others that stay loaded (0016) are busy pages too. A view
  wakes — `resumeView()` — when it next goes active, which is when it is on the screen:
  the one in front as the application comes back, the others when they are next
  brought to the front.
- A document that arrives while its view is asleep — a load that was under way, a
  redirect, a page that reloads itself — arrives in a new window that is not frozen,
  so every change of `loading` on a sleeping view puts it back to sleep, as
  sailfish-browser does with a page that finishes loading unseen. Only while the pages
  are asleep: a view behind the one in front is still suspended after the application
  is back, and suspending it then would stop the window the page on the screen draws
  into (below). A document that arrives in it then runs until the next sleep.

**Nothing is suspended while the application is on the screen.** The engine draws every
view into one shared window (`QMozContext::registeredWindow()`), `suspendView()` stops
that window drawing, and the only thing that starts it again is a view going active. A
view suspended as the grid switches tabs could land after the new one went active and
freeze the screen, and the order two views hear of one model change in is not ours to
set. While the pages are asleep nothing is drawn, and coming back is a view going
active.

## Consequences
A page left for more than a second comes back as it was, from a frozen script's point
of view no time having passed: timers fire late, a clock catches up, a live feed
refreshes. That is what the platform browser does. A page behind the one in front stays
frozen until it is shown, which on the first tap to it costs the thaw and nothing else.

What the engine calls a sound is a stream with an audio track; a video with a track
and the sound muted counts as sound and keeps pages awake. Media that plays from a page
already asleep, which a frozen page cannot start, is not handled. A page behind the one
in front never keeps anything awake: its view is inactive, so its media is paused the
moment it is left, which was so before this.

Out of sight the view in front still draws for as long as it is active — a second, or
the length of the sound — into a window nobody sees; the platform browser has a way to
stop that, and the Harbour `WebView` does not.

`WebView` has no Harbour-visible way to pause media on a call or a lock screen:
`Nemo.Policy`, which sailfish-browser takes its audio resource through, is not on
Harbour's import list.

The ten-minute `heap-minimize` of 0016 is unchanged; it asks the engine for memory,
this stops the pages using the processor. `tst_pageactivity` covers the timing and the
messages, `tst_qmlload::pagesSleepOutOfSight` what the page does with them; how much the
processor actually rests is on the device checklist.
