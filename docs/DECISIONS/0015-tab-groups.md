# 0015 — Tab groups: one grid per group, a strip to move between them

## Context
The grid showed every open tab in one field, and past a dozen or so it stopped being a
picture of anything: work beside reading beside the thing that was being looked up.
Safari on iOS answers this with tab groups — a named set of tabs, one shown at a time,
a row of names along the top of the switcher to flick between them, an edit sheet and a
search across all of them. That is the shape asked for here, and the one a reader of
that browser already knows.

## Decision
Every tab is in exactly one **group**. There is always a **default group**, the first
ordinary one, which can be neither renamed nor deleted, so the strip always reads
"*n* tabs" somewhere and the grid always has somewhere to be. A group has a name, which
may be empty: an unnamed group is shown as what it holds — "3 tabs" — the way Safari
shows the tabs outside every group, and it is what a database from before this record
turns into (every tab in group 1, unnamed).

The **tab model stays one list of every tab**, whatever its group. The browsing page
keeps one `WebView` per row of it (0003), so a tab changing group must not be a row
removed and inserted, or the page behind it would be reloaded; it is a change of one
role on a row that stays put. Two views are built on that list for QML:

- `GroupTabModel` (`GroupTabs`) — the current group's tabs in the model's own order,
  which the grid shows. It is a **list of ids the tab model keeps up to date**, not a
  `QSortFilterProxyModel`: a proxy answers a move in its source with a layout change,
  and a layout change makes Qt Quick rebuild every cell of the grid — including the one
  a finger is carrying (0010). With its own list the grid sees a move as a move.
  `GroupTabs.moveTab(from, to)` is a move in the tab model of the two tabs' rows, so
  what is persisted is the order the grid shows; the other groups' tabs, interleaved
  in the model, stay where they are.
- `TabGroupModel` (`TabGroups`) — the groups, in the order the strip shows them, with
  each one's tab count and whether it is current. The group actions are reached
  through it: `activate(row)`, `addGroup`, `renameGroup`, `removeGroup`, `moveTab`.

The **current group** (`TabModel.currentGroupId`) is the one the grid shows and the one
a new tab opens in. It follows the active tab — bringing a tab to the front from a
search result, or moving the tab in front to another group, moves the strip — and
choosing a group brings **that group's most recent tab** to the front, so what the page
shows when the grid is put away is the group that was chosen, as Safari's groups each
remember their own tab. An empty group leaves the page as it is. When the active tab
closes, the nearest tab in its own group takes its place, then the most recent tab
anywhere; a group can therefore be left empty and looked at empty. Deleting a group
closes its tabs, moving the current group to a neighbour first, so that whoever answers
"no tabs left" by opening one does not open it in the group that is going.

The **strip** (`components/TabGroupStrip.qml`) replaced the "*n* tabs" label in the
grid's head row, and has since moved to the **foot** row (below), between the new-tab
button in the left corner and the edit button in the right. The two corners are the same
width, so the names are centred on the screen. It is Silica's own `TabBar` geometry
rebuilt from public API, the way vuo rebuilds it: a `Flickable` over a `Row` of buttons,
each its name's width plus `Theme.paddingLarge` either side, the current name in the
highlight colour with a `Theme._lineWidth` underline exactly as wide as the name, the
first and last button taking the slack so a row that fits is centred, and the current
button kept in the middle when it does not. `TabBar` itself lives in
`Sailfish.Silica.private` and works only inside a `TabView`, neither of which a Harbour
application may have. A tap chooses a group. The first strip was a snapping `ListView`
with each item half the width, whose flick chose the group; it was too sparse to read as
a row of names, and the view's own writes to `currentIndex` during layout chose groups
nobody had asked for. The names are in medium type and the icons in the corners at
`Theme.iconSizeSmallPlus`, so the row sits with the search field at the head of the grid.
The names began in small type, as over the grid rather than at the head of a page, beside
icons of Silica's medium size; on device they were asked to match the field, whose large
type would make the names as big as a page's header, so the names went one step up
Silica's scale and the icons one step down. Each icon's button is a padding wider than it
either side and the row's height, the icon at the page margin. An end of the row with
names past it **fades out** rather than cutting a name off, as `TabBar`'s does: two
`OpacityRampEffect`s, one for each end, each on only while there are names out of sight
past its end and narrowing away as the row reaches it, the second drawn from the first
while both are on. `TabBar`'s fade is a seventh of the row; this one is at most a
twentieth of the screen, the ramp Silica puts on a field's text where it scrolls past an
end, since what was asked for was a slight one. A row that fits has none. The edit button
in the right corner pushes `pages/TabGroupsPage.qml`, a page over the grid, which stays
open under it.

The strip is at the **foot** so that it is within reach of the thumb that carries a cell
to it: a tab **changes group by being carried onto one**. A preview held until it comes
up (0010) and carried down over a name lights that name, in the wash the grid marks its
cells with, and dropped there the tab moves into that group. The cells ask the strip
through three functions — `carryOver()` while the finger moves, `dropTab()` as it lifts,
`endCarry()` however the carry ends — and a cell over the strip, its corners included,
trades places with none of the cells hidden under it. The current group is never lit:
the tab is in it already. Names scrolled out of the strip are not targets either. The
move itself waits a turn of the event loop: made at once, it takes the carried cell out
of the grid while that cell's own release handler is still running, the cell's context
is cleared under it, and the rest of the handler fails with a TypeError — which the
real-finger test (`tst_qmlload::carryToGroupUnderAFinger`) found, and now fails on.
Carrying the tab in front takes the grid with it, since the tab in front is always in
the group the grid shows; any other tab leaves the grid where it is, one cell the fewer.
The row is Silica's `TabBar` turned into a place to put things, which Silica has no
model for; the strip does not scroll itself while a cell is held over one end of it, so
with more groups than fit, a name out of sight has to be scrolled to first.

`TabGroupsPage` is a list with a tap to make a group current, rename and delete in each
row's menu, and under the last row a row shaped like a group's with a plus where its name
would start, which makes a group (`TabGroupDialog`, a name) — under the list rather than
in a pulley, the way postivene offers another profile, because that is where a reader who
has just read the list is looking. It was also a picker once, given a tab, for the menu's
"Move tab to group"; carrying the tab onto the strip replaced both (0021).

`TabSearchModel` (`TabSearch`) lists the tabs whose title or address contains the term,
case-insensitively, group by group in the strip's order. When the **term** changes the
rows are refined one at a time — kept, removed or inserted, walking the old rows and
the new together, both drawn from the same tabs in the same order — and never reset:
a reset rebuilt the list under the reader's finger on every keystroke, and on device
that threw the page about as the first results came in. When the tabs or the groups
change the list is built again, since the order it is walked in is no longer shared.
That was not the whole of it: the page still jumped at the first pause in typing. The
search field was the list's header, and a header lives inside the view's flickable,
whose content moves as the list narrows — and Silica takes the keyboard away when the
content under it moves. The field was then **anchored above the list**, outside it, and
the term reaches the model from a **250 ms timer** restarted on each keystroke, so a
burst of typing asks once; both are what postivene's chat search does, for the same
reasons.
The group heading is a role on the first row of each group rather than a section of
the list, so two unnamed groups holding the same number of tabs stay two headings.

The search was a page of its own, pushed by a button in the corner of the grid's head
row. It is now **the head row itself**: Silica's `SearchField`, "Search tabs", across
the row with its words from the left edge, and what it finds listed over the cells,
between the two rows, for as long as the term is not empty; the cells are not drawn
meanwhile, and emptying the field gives them back at once. The list is still not the
field's flickable. Enter puts the keyboard away, leaving the whole list to be seen. A
tap on a result brings that tab to the front and puts the grid away. The search belongs
to the grid while it is up: put away, however that happens, the field is emptied, the
keyboard goes and the term with it, so the next search starts empty over the cells. The
field is not focused when the grid comes up, so the keyboard comes only for a tap on it;
unfocused, it lets a drag down from it pull the page back, as the rest of the head row
does. `pages/TabSearchPage.qml` is gone.

Storage is schema 4: `tab.group_id`, a `tab_group` table, and `currentGroupId` beside
`activeTabId` in `setting`. `TabModel` repairs what it loads — a tab naming a group no
row describes gets that group created unnamed, which is exactly the pre-schema-4 case —
so no migration step writes a group row and the data cannot be left inconsistent by one
that fails half way.

## Consequences
`TabModel.count` and the cover still count every tab in every group; the cover's field
is unchanged. The grid's delegates address tabs by id (`activateTabById`,
`closeTabById`) rather than by row, because the grid's rows are no longer the model's.

There is no way to reorder groups yet: the default group is first and a new group
goes last. Two unnamed groups are told apart in the strip only by their counts. The
private group that once sat before the default one is gone (0019).

`components/TabsView.qml` keeps its size by handing the strip its own file. The load
tests drive the strip through `select(index)`, which is what a tap calls, and through
`dropTab()` with `dropIndex` set; the carry itself is tested under a real finger. The
pages are driven through their `objectName`s as the other pages are.
