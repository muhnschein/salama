# 0026 — A tab's mute on the preview, the bar and the cover; what plays is the front tab's

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
observer's topic and data without its subject.

What the platform allows to play. **Only the tab in front can.** Firefox suspends the
media of a document that is not active — one kept for the back button — and, when the
embedder asks for it (GeckoView's `suspendMediaWhenInactive`, off unless set), of one
whose browsing context is inactive. Sailfish's Gecko changes the first test to a
*hidden* document (sailfishos/gecko-dev,
`rpm/0056-Ensure-audio-continues-when-screen-is-locked.-Contri.patch`), and the view of
a tab behind the one in front is inactive, which hides its document: its media is
suspended the moment it is left, `paused` untouched, and goes on when it is shown again.
Keeping that view active does not help. embedlite draws every view through the one
window's renderer (`EmbedLitePuppetWidget::GetWindowRenderer` borrows the top-level
widget's), so a second active view paints into the same WebRender layer manager as the
one on the screen: a page playing behind would draw over the page in front.
sailfish-browser has the same limit and does the same, suspending the page it leaves.
A page may also pause itself on being hidden — YouTube's mobile site does — and then
nothing brings it back when it is shown.

Firefox for Android plays on in a tab left for another and in the background. The
video of a hidden or unseen `<video>` stops being decoded after ten seconds
(`media.suspend-bkgnd-video.*`, on by default and not changed by embedlite's prefs),
so what goes on is the sound. With the tab in front kept active while it plays out of
sight (0020), the document is not hidden and that never happens here: the view goes
on decoding and drawing pictures nobody sees.

## Decision
**One control, the mute, and muting pauses.** The first build had two, play/pause as
Firefox for Android had it and mute as Firefox and Safari have it; on device, two
glyphs were one more than a preview or the bar could carry, and a muted page left
playing was still going when it was wanted back. So a muted tab is paused as well, and
unmuted it plays again what that paused. The control shows while the tab's page plays
something or was paused from here, and whenever the tab is muted.

**It shows whether the tab is heard**, not only whether it was muted: the speaker while
its page plays in front and is not muted, struck through otherwise — muted, paused, or
held behind the front (`PageMedia::isHeard`). Showing the mute flag alone left an
unmuted speaker over every tab paused as it was left, with nothing playing anywhere. A
tap on a tab that is heard mutes and pauses it; on one that is not, it unmutes it and
plays what was paused — and a tab behind the front is brought to the front for that,
the one place it can play, with the grid left open.

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
  one quick action the reader chose (0014, 0029), and the only one when the reader chose
  none; the quick action's place is the first, the mute's the second, and the cover's
  settings say so. The home screen draws an action's picture from its file as it is, so
  the two speakers are drawn for it (`icons/cover/`, rendered by `icons/render.sh` into
  `art/cover/` at each size Silica's small icon takes, in white and in black), as piirit
  draws its own, and the quick action's glyphs are drawn beside them; the theme's
  `icon-cover-mute` is not used, its glyph unchecked.
- The glyphs are `icon-m-speaker-on` and `icon-m-speaker-mute`, those of the Harbour
  players that mute (Jupii, harbour-sailfishconnect). Neither has a small size, so they
  are drawn smaller.

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

**A tab left while it plays is held.** The tab model says a tab is about to be left
before anything else hears of it (`TabModel::activeTabLeaving`), and `PageMedia` pauses
the page then, marked as it pauses everything, while its view is still the one in front:
the message goes out on the same channel as the one that makes the view inactive, and
ahead of it, so the page is paused before it is told it is hidden and has no pause of its
own to make. When the tab is back in front, what was held is played again. A page that
starts loading meanwhile takes the hold with it; one muted from here is not held, and
comes back paused. A page behind the front that says it plays anyway — one the script
could not reach in time — is shown paused (`TabModel::shownMediaState`), since it is
silent.

**One tab plays at a time**, the one in front. When it answers that it plays, every other
tab whose page says it plays is paused, for real — brought back, it stays paused — and a
tab behind that says it plays while the front does is paused too. What a page answers to
being paused asks nothing more: a page that will not pause is asked again on the engine's
next word, not in a loop.

**Out of sight, what plays is its sound.** While the application is out of sight
(`PageActivity.background`), the script hides every `<video>` that plays — its own
`visibility`, so the page's layout does not move — and puts back what the page had set
once the application is back, at once rather than with the engine's words. A video
nobody can see is one Gecko stops decoding the pictures of, ten seconds on
(`media.suspend-bkgnd-video.*`), and the page has nothing left to redraw; the sound plays
on. Whether a video hidden by its style counts as unseen on this engine, as one scrolled
away does, is on the device checklist.
The view in front stays active, as 0020 has it: an inactive one would be a hidden
document, and silent.

A frame script was considered: `loadFrameScript` takes a local or `data:` url and runs it
with the engine's own privileges, where Gecko's `audio-playback` topic names the page
that sounds and the browsing context can mute all of it. It is not used: privileged code
bound to Gecko's internals, which change with every ESR, and none of it can be verified
without a phone (SCOPE.md §7).

## Consequences
Media the page's own scripts cannot reach has no controls: a player in a frame from
another site (a video embedded in an article), a `new Audio()` never put in the document,
Web Audio. It still keeps the pages awake out of sight (0020).

A muted tab's new media is muted when its page is next asked, a fifth of a second and a
script's run after the engine's word; the start of it can be heard. A page can unmute or
resume its own media; the next ask shows it as it is. Mute does not survive a restart,
which Firefox's does. The marks are properties on the page's elements, which the page can
see.

Music in one tab stops while another is read, and starts again when its tab is back in
front: no tab behind the front plays, which is the engine's rule, not a choice here. A
Harbour application cannot change it; Gecko could, by suspending a hidden document's
media only where the embedder asks, and embedlite, by not painting a view that is not
in front.

There is no pause without the mute, nor a way to resume a tab paused for another's sake
but the page's own player or the mute's struck speaker. A page that hides and shows its
own videos out of sight may find its `visibility` set back to what it was when the
application went away.

`tst_pagemedia` runs the script over a page made of the parts of the DOM it reads;
`tst_qmlload::mediaControls` drives the bar, the cover and a tab being left and coming
back, `muteOnTheGrid` the previews. What a real page answers, and
how quickly, is on the device checklist.
