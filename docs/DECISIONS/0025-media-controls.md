# 0025 — A tab's mute on the preview, the bar and the cover; one tab plays at a time

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
**One control, the mute, and muting pauses.** The first build had two, play/pause as
Firefox for Android had it and mute as Firefox and Safari have it; on device, two
glyphs were one more than a preview or the bar could carry, and a muted page left
playing was still going when it was wanted back. So a muted tab is paused as well, and
unmuted it plays again what that paused. The control shows while the tab's page plays
something or was paused from here, and whenever the tab is muted.

- On a preview in the grid (`PreviewMuteAction`), drawn as a cover draws its quick
  actions: the glyph alone, centred along the picture's foot, a step larger than the
  small icons. Under it the picture fades out towards its bottom edge over twice the
  action's height (`FootFade`), so the glyph sits on the cell's own ground rather than
  on the page — how piirit's and vuo's covers make room for what they draw at their
  foot, eased as piirit's is. It keeps its own taps; the cell under it does not open,
  and does not come to the front.
- On the bar (`AddressLabel`), the glyph hangs off the left of the host rather than
  being part of the row the bar centres, which is the host and the TLS warning (0011):
  counted in, it pushed the host off the middle of the bar whenever something played.
  It is drawn in the ambience's colour where the bar's other controls are in the
  primary one, a step larger than the small icons, and shrinks with the host on the
  slim bar, where a tap brings the whole bar back, as any tap on it does (0009). What
  lies between back's region and the host is its to tap.
- On the cover, while the tab in front plays or is muted, a second action beside the
  search (0014). The home screen draws an action's picture from its file as it is, so
  the two speakers are drawn for it (`icons/cover/`, rendered by `icons/render.sh` into
  `art/cover/` at each size Silica's small icon takes, in white and in black), as piirit
  draws its own; the theme's `icon-cover-mute` is not used, its glyph unchecked.
- The glyph says what the tab's sound is now: the speaker while it is on, struck
  through while muted — `icon-m-speaker-on` and `icon-m-speaker-mute`, those of the
  Harbour players that mute (Jupii, harbour-sailfishconnect). Neither has a small size,
  so they are drawn smaller.

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
is shown paused (`TabModel::shownMediaState`), since it is silent. Muted and unmuted from
its preview, it stays where it is: what unmuting plays again is held until the tab comes
to the front, where the engine lets it go on.

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

There is no pause without the mute, nor a way to resume a tab paused for another's sake
but the page's own player. A tab paused so shows its mute, unmuted.

`tst_pagemedia` runs the script over a page made of the parts of the DOM it reads;
`tst_qmlload::mediaControls` drives the bar, the grid and the cover. What a real page answers, and
how quickly, is on the device checklist.
