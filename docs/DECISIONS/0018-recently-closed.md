# 0018 — Recently closed tabs come back from under the grid's foot

## Context
A tab closed by a slip of the finger — and with flick-to-close on the grid (0010)
there is a new way to slip — was gone. The person testing asked for what every
desktop browser has: a list of what was closed lately, to open again.

## Decision
`ClosedTabModel` (`ClosedTabs`) keeps the last thirty tabs closed, newest first, each
as the address, title and favicon it had, in a `closed_tab` table (schema 5) so that
the list survives a restart. `TabModel` records every tab it closes, one at a time or
all at once. `reopen(row)` opens the tab again in the current group, gives it back its
title and icon so the grid's cell can say them before the page has loaded, and takes
it off the list.

The list is shown by **holding** the new-tab button, in the left corner of the grid's
foot row (0010) — where it started, and where it came back to after a while in the head:
a tap still opens a tab, a hold brings up `components/RecentlyClosedPanel.qml`, a Silica
`DockedPanel` docked to the bottom and modal, so it slides up from under the foot the way
the platform's own sheets do and a tap outside it puts it away, as does pulling it back
down. The list is read and tapped with the thumb, and the browser's menu is the same
kind of sheet from the same edge (0021). Its rows are
`components/TabRow.qml`, the same row the search results are made of, so the two
lists read the same. A tap on a row opens the tab, hides the panel and hands the page
back through the grid's `tabActivated`.

## Consequences
Thirty is a number: enough to undo an afternoon, not a history. The panel is part of
the grid rather than a page of its own, so it does not move the page stack and the
grid stays where it was when the panel goes.

`tests/silica-stubs` gains a `DockedPanel` and the `Dock` enum, on the terms the other
stubs are written on: the panel's `open` flips on `show()` and `hide()` with no
animation, and how it actually slides is a device question. Open, the stub lies along
the edge it is docked to, where Silica's own settles, so that a test can put a finger on
it (0021).
