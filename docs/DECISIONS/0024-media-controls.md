# 0024 — Media controls on the preview and the bar; one tab plays at a time

## Context
A page playing something said so nowhere but on itself: the grid did not say which tab
the sound came from, and stopping it meant finding that tab and the page's own player.

What other browsers do. Firefox puts a speaker button on the tab that mutes and unmutes,
never pauses, and keeps the tab muted across its navigations. Firefox for Android put a
play/pause button in the corner of a tab's thumbnail, and has no mute anywhere. Safari
has its mute in the address field; Chrome's toolbar has a button with play and pause.
None of them puts mute and pause in one control. The mobile ones — Firefox and Chrome
for Android — let one tab play at a time; the desktop ones do not.

What the platform gives. `QuickMozView` has no property, signal or slot about audio,
media or muting. The engine's `media-decoder-info` (0020) says a decoder started or
stopped and names it by its address, not by its window, and embedlite forwards an
observer's topic and data without its subject. Gecko holds a hidden document's media —
the view of a tab behind the one in front is inactive, and its document hidden — without
touching the element's `paused`, and plays it on when the document is shown again.

## Decision
**Two controls, on both surfaces**, separate as everywhere else: play/pause as Firefox
for Android had it, mute as Firefox and Safari have it.

- On a preview in the grid (`PreviewMediaControls`), two discs in the picture's
  bottom-left corner, drawn as the close button's disc is (`PreviewButton`): play/pause
  while the page plays or was paused from here, and mute then too, or whenever the tab
  is muted. They keep their own taps; the cell under them does not open.
- On the bar, the same two glyphs (`MediaIcon`) left of the host, in the row the TLS
  warning is in (0011), and on the slim bar too, as the warning is — where a tap brings
  the whole bar back, as any tap on it does (0009). What lies between back's region and
  the host is theirs to tap.
- The glyphs say what the page does now: pause while it plays, the speaker struck
  through while muted. `icon-m-play` and `icon-m-pause` are Jolla's media controls';
  `icon-m-speaker-on` and `icon-m-speaker-mute` those of the Harbour players that mute
  (Jupii, harbour-sailfishconnect). None has a small size, so they are drawn small.

**What plays is asked of the pages** (`src/engine/PageMedia`). `PageActivity` tells it
when a decoder's play state changes, and so does the tab model when another tab comes
to the front; a fifth of a second after the first word, every loaded page is asked once,
the view running `PageMedia.script()` with `runJavaScript` as it runs the favicon's
(0006) and handing the answer back. The script finds the `<audio>` and `<video>` of the
document and of its frames from the same site, applies the tab's mute, carries out a
pause or a play, and answers `playing`, `paused` or nothing. Only what is heard counts:
an element the page muted or turned down to nothing, or a video Gecko says has no sound
(`mozHasAudio`), is left alone. What the script mutes, it marks, and unmutes only that;
what it pauses, it marks, and play resumes only that — the button undoes its own pause
and starts nothing the page has not. The answer is the tab model's `mediaState` role; a
page that is loading, a view given up (0016) or a tab closed plays nothing. `muted` is
the tab's, kept while it is open and applied each time its page is asked.

**Behind the front, playing is held.** A page behind the one in front that says it plays
is shown paused (`TabModel::shownMediaState`), since it is silent; its play button brings
the tab to the front, where the engine lets it go on, and plays what was paused from
here. The grid stays open.

**One tab plays at a time**, the one in front. When it answers that it plays, every other
tab whose page says it plays is paused, for real — brought back, it stays paused — and a
tab behind that says it plays while the front does is paused too. What a page answers to
being paused asks nothing more: a page that will not pause is asked again on the engine's
next word, not in a loop.

A frame script was considered: `loadFrameScript` takes a local or `data:` url and runs it
with the engine's own privileges, where Gecko's `audio-playback` topic names the page
that sounds and the browsing context can mute all of it. It is not used: privileged code
bound to Gecko's internals, while the platform moves from ESR 91 to 115, and none of it
can be verified without a phone (SCOPE.md §7).

## Consequences
Media the page's own scripts cannot reach has no controls: a player in a frame from
another site (a video embedded in an article), a `new Audio()` never put in the document,
Web Audio. It still keeps the pages awake out of sight (0020).

A muted tab's new media is muted when its page is next asked, a fifth of a second and a
script's run after the engine's word; the start of it can be heard. A page can unmute or
resume its own media; the next ask shows it as it is. Mute does not survive a restart,
which Firefox's does. The two marks are properties on the page's elements, which the page
can see.

`tst_pagemedia` runs the script over a page made of the parts of the DOM it reads;
`tst_qmlload::mediaControls` drives the bar and the grid. What a real page answers, and
how quickly, is on the device checklist.
