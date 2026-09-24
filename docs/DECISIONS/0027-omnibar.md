# 0027 — The address bar searches the tabs, bookmarks, history and downloads

## Context
Typing into the address bar went to an address or searched the web, and nothing else: a
page open in a tab of another group, bookmarked, visited last week or downloaded had to be
found on a page of its own, and the grid's search matched in a way of its own. What was
asked for is an omnibar: once something is typed, the room above the keyboard offers the
address, a search with the chosen engine, and what the phone already holds that matches —
laid out as piirit lists what its search of the chat list finds, a section per kind under a
heading that counts it. Firefox's address bar suggests from the open tabs, the bookmarks
and the history, each a source that can be switched off, ranked by frecency; opened for a
new tab, sailfish-browser lists its favourites before anything is typed.

## Decision
**The pane** (`components/OmnibarView.qml`) is up while the address is edited and either
what is typed differs from the url the field opened with, or the bar was opened for a new
tab (`NavigationBar.paneUp`). It fills the room from under the cutout to the bar, over the
page, and holds, from the bottom up:

- **the actions**, fixed directly above the bar, in reach of the thumb that typed
  (`OmnibarAction`): *Go to* the address, with the url it resolves to under it, when
  `Settings.isAddress()` says the text is one — true exactly when `urlForInput()` would
  not make a search of it — and *Search <engine> for “…”* whenever something is typed, a
  search even of an address. They follow the text at once. Enter means what it meant.
- **the results**, a `SilicaListView` over the `Omnibar` model sectioned by kind — Tabs,
  Bookmarks, History, Downloads — under headings that count them, "(10 of 34)" when a
  section shows fewer than it found. Every row is drawn alike (`OmnibarResultRow`): the
  site's icon or a theme glyph, the title, under it the host — for a tab in another group,
  that group's name first; for a download, how it is going — and on the right a date for
  the history and downloads. The list hangs from the actions and is as tall as what it holds, so a
  short one sits by the bar; it is not laid out bottom-up, so its sections read from the
  top as every list on the platform does. Its `currentIndex` is -1, as sailfish-browser's
  history list has it, so the model never takes the field's focus. Empty, it is absent.
- **the ground**, a pane of the grid's glass (0010), opaque as its rows are, that takes
  every press: nothing of the page is seen or reached through it, and a tap on the bare
  glass ends the edit.

A tab chosen comes to the front with its group; a bookmark or a page of the history opens
in the tab in front; a finished download opens its file, and one still coming or failed
shows the downloads. Opened for a new tab — the cover's search (0029) — the field is empty
and the pane lists the bookmarks at once, as sailfish-browser does; a page chosen or an
address entered opens in a new tab (a tab chosen still comes to the front), and a tap on
the glass leaves none made.

**Focus and the keyboard.** Silica's field lets its focus go at any press outside it, and
the bar ended the edit on focus lost and on the keyboard closing, which with a list to
scroll above the bar would end every edit at the first touch on it. So while the pane is
up the field's `focusOutBehavior` is `KeepFocus` (`AddressField.keepsFocus`), set by a
Binding that is always there and only changes its value: on Qt 5.6 one that lets go leaves
a plain value as it set it, and the field would go on keeping its focus after the pane.
And the keyboard closing ends nothing: the field drops its focus with it, so that a tap on
the field brings both back, and the pane grows into the room Silica gives back. Dragging
the list puts the keyboard away, as Firefox for Android does with its suggestions, so the
list has that room. The edit ends on Enter, a row or an action chosen, a tap on the glass,
the menu, or the grid coming up. With the pane down, everything is as it was (0009).

**Stacking and the reach.** The bar's gesture reaches a little above the bar, to hand taps
to the page and take a drag up for the grid (0009). The pane lies there, so while it is up
the reach is none (`BarGesture.reaching`), and the pane is declared after the bar and the
find bar, so nothing of theirs is drawn over it. The grid is still pulled from the bar.

**The model** (`src/omnibar/OmnibarModel`, the `Omnibar` singleton) lists the open tabs of
every group but the one in front — the page the bar is over — and the bookmarks, the
history and the downloads that hold every word typed. Within a section the rank is stable
over the source's own order: a host that begins with the first word, then a title with a
word that does, then the rest. The source's order is the tabs most recently in front
first, the bookmarks' own, the downloads newest first, and the history by frecency —
visits weighted by the age of the last, 100 within four days down to 10 past three months,
as Firefox weighs visits by age without keeping each visit to weigh. A bookmark open in a
listed tab is left out, and a page of the history listed as either: one row a page, the
nearest to hand. Ten rows a section, five downloads, is more than the pane shows unscrolled
— a first row behind three sections of fifty is no suggestion — and the heading says how
many more there were. Each source can be switched off in Settings > Search (0028), as
Firefox's can; the new-tab bookmarks follow that switch.

The history is matched in C++ over its whole table, not the model's page of 500 and not
by SQLite's `LIKE`, whose case folding is ASCII's: the table is pruned to 2000 rows, so a
read of it per rebuild is affordable, and case folds by Unicode's tables in every source —
"äly" finds "Älypuhelin".

**One matcher.** `src/search/SearchWords` takes what is typed as words, and a candidate
matches when it holds every word, in one field or another: "news helsinki" finds a page
called "Helsinki news". The grid's "Search tabs" (`TabSearchModel`, 0015) matched the whole
text as one piece; it uses the same matcher now, so the two find the same tabs.

**Timing.** The query is written 150 ms after the last key, sooner than the grid's 250:
here the first row found is the next tap, where the grid's search narrows a list already
on the screen. A source that changes while there is a query rebuilds once on the next turn
of the event loop, for however many changes came together — a page loading reports its
address, title, icon and visit. A rebuild that comes to the same rows, kind and id for id,
changes them in place, so a download's progress or a tab's icon arriving never tears down
the row under a finger.

**Not done:** suggestions from the search engine. They would send every key typed to it,
where everything the pane shows now stays on the phone.

## Consequences
`urlForInput()` and `isAddress()` share one rule, which moved one edge: a host with an
impossible port, "example.org:99999", was neither address nor search and Enter did
nothing; it is a search now.

`BrowserPage.qml` had no room for the pane, so the deck's state and gestures went to
`components/TabDeck.qml` first (0010), and the field went from `NavigationBar.qml` to
`AddressField.qml` to keep the bar under 400 lines.

`tst_searchwords` and `tst_omnibarmodel` test the matcher and the model; `omnibar`,
`omnibarFollowsItsSources`, `omnibarChoices` and `omnibarForANewTab` in `tst_qmlload`
drive the pane and the bar through the stubs, which gained Silica's `FocusBehavior`.
Whether the field keeps its focus through taps and scrolls on the pane, how the list
settles its height, and how the pane looks are device checks (`docs/TESTING.md`).
