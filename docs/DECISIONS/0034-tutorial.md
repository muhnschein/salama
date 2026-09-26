# 0034 — A tutorial teaches the way to the tabs

## Context
The tab grid has no button. It lies under the page, and the navigation bar dragged upwards
brings it up (0009, 0010); the menu's *Tabs* entry, which was the way in without the
gesture, went when the menu became a sheet (0021). The handle on the bar's edge says the
bar can be taken hold of, but not what is under it, and a reader coming from
sailfish-browser, whose toolbar has a tabs button, or from any other browser, has no reason
to try. A reader who never drags the bar never finds the tabs.

The platform teaches its own gestures in two ways. Jolla's applications play an animated
hint in place the first few times a view is used — Silica's `TouchInteractionHint`, a glow
travelling the way the finger is to go, and `InteractionHintLabel` saying it, counted in
dconf by `FirstTimeUseCounter` (jolla-gallery's `VerticalPageBackHint.qml`, jolla-email's
`FolderAccessHint.qml`). The Tutorial the first boot shows (`Sailfish.Tutorial`) teaches
the system's gestures on a picture of the screen that answers the finger, one lesson at a
time: a start card with *Start*, then each step shown by the same two Silica types and
waited for until the finger has made it, the hint and the words put away while a finger is
down, and after a pause of 800 ms a card saying *Well done!* with the lesson said back,
*Try again* and, after the last, *Close tutorial*, over the screen dimmed to nine tenths of
`highlightDimmerColor`. Its `LauncherLesson` is this browser's gesture on the home screen:
"Swipe from the bottom edge to open the app grid", then "Swipe from the top edge to close
it", the hint going up and then down. sailfish-browser shows no hints at all.

## Decision
**A tutorial page, as the Tutorial runs a lesson** (`pages/TutorialPage.qml`). A card says
what it is for: "Learn where your tabs are", that there is no tabs button, and *Start*. Then
the lesson, on a sketch of the browser: "Drag the bar up to see your tabs", with a
`TouchInteractionHint` going up from the bar's handle, where the real drag is aimed (0010);
once the grid is up, "Pull down to go back to the page", the hint going down from the upper
third, where Silica starts a pull; once the page is back, 800 ms later, *Well done!*, "Now
you know where your tabs are", *Try again* and *Close tutorial*. The words are at the other
end of the screen from the movement — the band inverted along the head while the bar is the
thing — and, with the hint, go while a finger is on the screen. A drag let go short of the
threshold springs back and leaves the step as it was. The hint is a `Pull`, the pulley's
movement, drag and release: not an `EdgeSwipe`, since the drag has to start on the bar,
above the edge the system keeps for itself (0009).

**The deck is the real one.** `components/TabDeck.qml` carries the sketch, and the bar's
drag is the real `BarGesture` over a picture of the bar (`components/TutorialBar.qml`) —
its handle, its reach above the bar, its threshold, its spring — so the gesture learnt is
the gesture used. The grid is drawn (`components/TutorialGrid.qml`): its glass rows, the
line across its top, cells washed as the grid washes them, and the way back its own
overscroll, as the real grid's is. It is not the browsing page's own deck with hints laid
over it: that page is at the 600 lines nothing waives (0010), a hint over the real grid is
over the reader's own tabs, where a stray tap closes one, and nothing done in a sketch can
be done to a tab.

**It comes up once by itself, and from Settings whenever asked.** The root window pushes
it over the browsing page, at once rather than sliding in, on the first turn of the event
loop after the window is made, while `Settings.tutorialShown` is off, and turns that on as
it does: a first start, and the first start of a build that has it. A tutorial left by back
has been shown; it is not forced on the reader again. Settings ends with *Help*, as Firefox
for Android ends its settings with its own, holding *Tutorial* with `icon-m-gesture`, the
theme's icon for a gesture — Jolla's Settings has no row for its Tutorial to take one from,
the Tutorial being opened again from the app grid. Opened from Settings, it goes back to
Settings when closed.

**Not `FirstTimeUseCounter`.** The counter keeps its count in dconf, outside the one
settings file this application writes (ARCHITECTURE.md), and would need
`Nemo.Configuration` stubbed for the tests. It also waits for the system's own hints on a
new device, and stays silent for good once Settings > Gestures > *Show hints and tips* is
off; that switch speaks for the platform's hints, which teach what every application
shares, and this is the one gesture that is this browser's alone.

## Consequences
The first thing a first start shows is the tutorial rather than the start page, and a
reader who knows the gesture pays one swipe back for it. `tests/tst_qmlload.cpp` marks the
tutorial shown for every test but `tutorialOnFirstStart`, which checks that a first start
shows it and the next does not; `tutorial` drives the steps and cards, and
`tutorialUnderAFinger` makes both gestures in a real window. The stubs gain
`TouchInteractionHint`, `InteractionHintLabel`, `FadeAnimation`, and the `TouchInteraction`
and `PageStackAction` enums, with Silica's values. How the hint looks and moves, and
whether the sketch reads as the browser, are device checks (`docs/TESTING.md`).
