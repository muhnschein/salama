# 0027 — The address bar searches the tabs, bookmarks, history and downloads

## Context
Typing into the address bar went to an address or searched the web, and nothing else:
a page already open in a tab, bookmarked, visited or downloaded had to be found on a
page of its own.

## Decision
While the address is edited, a pane above the bar lists what the typed words find in
the open tabs of every group, the bookmarks, the history and the downloads, a section
of each, with a row to go to the address or search for the words kept in reach above
the bar. `src/omnibar/OmnibarModel` builds the list, with the one word matcher,
`src/search/SearchWords`, that the grid's tab search now uses too.

## Consequences
The history is searched in C++ over its whole table, which is bounded, so that case
folds by Unicode's rules in every source; each source can be switched off in Settings.
