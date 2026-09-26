# 0034 — A tutorial teaches what this browser does its own way

## Context
Three things here work unlike other browsers, and a fourth has no button at all. The
address bar is one field for an address and a search (0027). The menu is a sheet of icons
under the bar (0021). The tab grid lies under the page, and the navigation bar dragged
upwards brings it up (0009, 0010); the menu's *Tabs* entry, the way in without the
gesture, went when the menu became a sheet. In the grid, a tab is closed by sliding it
away, moved by holding and carrying it, and put in another group by carrying it onto the
group's name (0010, 0015). The handle on the bar's edge says the bar can be taken hold of,
but not what is under it, and a reader coming from sailfish-browser, whose toolbar has a
tabs button, has no reason to try.

The platform teaches its own gestures in two ways. Jolla's applications play an animated
hint in place the first few times a view is used — Silica's `TouchInteractionHint`, a glow
travelling the way the finger is to go, `TapInteractionHint`, a blink where it is to tap,
and `InteractionHintLabel` saying it, counted in dconf by `FirstTimeUseCounter`
(jolla-gallery's `VerticalPageBackHint.qml`, jolla-email's `FolderAccessHint.qml`). The
Tutorial the first boot shows (`Sailfish.Tutorial`) teaches the system's gestures on a
picture of the screen that answers the finger, one lesson at a time: a start card, then
each step shown by those Silica types and waited for until the finger has made it, the
hint and the words put away while a finger is down, and at the end a card with the lesson
said back and *Close tutorial*, over the screen dimmed to nine tenths of
`highlightDimmerColor`. Its `LauncherLesson` is this browser's grid gesture on the home
screen: the app grid opened from the bottom, then closed from the top, the hint going up
and then down. Explanatory steps have a button to go on. sailfish-browser shows no hints
at all.

## Decision
**A tutorial page, as the Tutorial runs its lessons** (`pages/TutorialPage.qml`), on a
sketch of the browser, a step at a time:

1. *The address bar*: a tap on it, which turns it into a field with an address typed in and
   the pane above it — a row to go to the address and a row to search for it, the pane's
   own `OmnibarAction`, naming the engine Settings chose — explained, and *Continue*.
2. *The menu*: a tap on its button, which brings up the sheet with the real sheet's icons
   and names (`MenuButton`), and a tap outside it, which puts it away.
3. *The grid*: the bar dragged up, the hint a `Pull` going up from the bar's handle, where
   the browsing page's drag is aimed (0010) — not an `EdgeSwipe`, since the drag starts on
   the bar, above the edge the system keeps for itself (0009).
4. *In the grid*: a tab slid to the left, which closes it; a tab held and carried over
   another, which trades places with it; a tab carried onto the other group's name, which
   moves it there.
5. *Back*: the grid pulled down past its top, the hint a `Pull` down from the upper third,
   where Silica starts a pull.

Then, 800 ms later so the page is seen to come back, *Tutorial complete*, that Settings has
it, and *Close tutorial*. The words are at the other end of the screen from the gesture,
the band inverted along the head where the gesture is at the foot, and, with the hint, go
while a finger is on the screen. A gesture made out of turn does nothing — the bar cannot
be dragged up before its step, nor the grid pulled down — and one let go short of its
threshold leaves the step as it was. A hint is placed where its target will be once the
deck is at rest, since a step changes the moment a finger lifts, while the deck is still
springing into place (`components/TutorialHints.qml`).

**The sketch moves as the browser does.** `components/TabDeck.qml` carries it, and the bar's
drag is the real `BarGesture` over a picture of the bar (`components/TutorialBar.qml`). The
grid's cells are the real grid's, `TabPreview`, over a model of four made-up pages drawn by
`icons/render.sh` into `art/tutorial/`, with a strip of two groups
(`components/TutorialStrip.qml`) that answers a carried cell as the real strip does, through
`carryOver()`, `dropTab()` and `endCarry()`. So the hold, the slide and the carry learnt are
the ones used; nothing done to a sketch is done to a tab. The browsing page itself carries
no hints: it is at the 600 lines nothing waives (0010), and a hint over the real grid is
over the reader's own tabs.

**The words** follow the rules piirit and vuo keep for strings that go to translators: a
finished sentence with a subject and a verb, literal verbs, sentence case, no figure a
translator could take literally. The strings the tutorial shares with the browser — the
menu's names, the omnibar's rows, "%n tab(s)" — are the same source text, and their
translations are the browser's.

**It comes up once by itself, and from Settings whenever asked.** The root window pushes it
over the browsing page, at once rather than sliding in, on the first turn of the event loop
after the window is made, while `Settings.tutorialShown` is off, and turns that on as it
does: a first start, and the first start of a build that has it. That first time it opens
on a card as piirit's first screen does — the launcher icon drawn larger (`art/logo.png`),
the name, "Web browser for Sailfish OS", one line on what the tutorial covers — with
*Start tutorial* and *Skip*. A tutorial skipped or left by back has been shown. Settings
ends with *Help*, as Firefox for Android ends its settings, holding *Tutorial* with
`icon-m-gesture`; opened from there it starts at the first lesson, and goes back to
Settings when closed.

**Not `FirstTimeUseCounter`.** The counter keeps its count in dconf, outside the one
settings file this application writes (ARCHITECTURE.md). It also waits for the system's
own hints on a new device, and stays silent for good once Settings > Gestures > *Show
hints and tips* is off; that switch speaks for the platform's hints, which teach what every
application shares.

## Consequences
The first thing a first start shows is the first card rather than the start page, one tap
from either. `tests/tst_qmlload.cpp` marks the tutorial shown for every test but
`tutorialOnFirstStart`, which checks the card, *Skip*, and that the next start shows
nothing; `tutorial` walks every step and what each shows, and `tutorialUnderAFinger` makes
every gesture in a real window. The stubs gain `TouchInteractionHint`, `TapInteractionHint`,
`InteractionHintLabel`, `FadeAnimation`, `Theme.fontFamilyHeading`, and the
`TouchInteraction` and `PageStackAction` enums, with Silica's values. How the hints look and
where they start, and whether the sketch reads as the browser, are device checks
(`docs/TESTING.md`).
