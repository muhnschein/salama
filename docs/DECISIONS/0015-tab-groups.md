# 0015 — Tab groups: one grid per group, a strip to flick between them

## Context
The grid showed every open tab in one field, and past a dozen or so it stopped being a
picture of anything: work beside reading beside the thing that was being looked up.
Safari on iOS answers this with tab groups — a named set of tabs, one shown at a time,
a row of names along the top of the switcher to flick between them, an edit sheet and a
search across all of them. That is the shape asked for here, and the one a reader of
that browser already knows.

## Decision
Every tab is in exactly one **group**; there is always at least one, and the last
cannot be deleted. A group has a name, which may be empty: an unnamed group is shown as
what it holds — "3 tabs" — the way Safari shows the tabs outside every group, and it is
what a database from before this record turns into (every tab in group 1, unnamed).

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
grid's head row: a horizontal `ListView` over `TabGroups`, `SnapOneItem` with
`StrictlyEnforceRange` so the current group sits in the middle with half a neighbour to
either side, each item half the list's width. Its `currentIndex` and the model's current
group each drive the other; the list's index is set from a `Connections` rather than
bound, because the view writes to it too and a binding would not survive that. The edit
button in the left corner pushes `pages/TabGroupsPage.qml`; the search button in the
right corner pushes `pages/TabSearchPage.qml`. Both are pages over the grid, which
stays open under them.

`TabGroupsPage` is a list with a pull-down to make a group (`TabGroupDialog`, a name),
a tap to make one current, and rename and delete in each row's menu. Given a tab
(`moveTabId`) it is a picker instead: the tap moves the tab into that group, and the
pull-down makes a new group with the tab in it. The menu's "Move tab to group" opens it
that way for the tab in front — the one way a tab changes group, beside the grid it
leaves rather than on a cell that already carries a tap, a carry and a close button.

`TabSearchModel` (`TabSearch`) rebuilds its rows on every change to the tabs or the
groups: title or address containing the term, case-insensitively, group by group in the
strip's order. The group heading is a role on the first row of each group rather than a
section of the list, so two unnamed groups holding the same number of tabs stay two
headings. A tap calls `BrowserPage.showTab(id)`: the tab to the front, the deck settled
on the page.

Storage is schema 4: `tab.group_id`, a `tab_group` table, and `currentGroupId` beside
`activeTabId` in `setting`. `TabModel` repairs what it loads — a tab naming a group no
row describes gets that group created unnamed, which is exactly the pre-schema-4 case —
so no migration step writes a group row and the data cannot be left inconsistent by one
that fails half way.

## Consequences
`TabModel.count` and the cover still count every tab in every group; the cover's field
is unchanged. The grid's delegates address tabs by id (`activateTabById`,
`closeTabById`) rather than by row, because the grid's rows are no longer the model's.

There is no way to reorder groups yet, and no private group; the second is the subject
of the research that followed this record. Two unnamed groups are told apart in the
strip only by their counts.

`components/TabsView.qml` keeps its size by handing the strip its own file. The load
tests drive the strip through its list's `currentIndex`, which is what a flick changes,
and the pages through their `objectName`s as the other pages are driven.
