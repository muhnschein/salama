# 0021 — The menu is a sheet of icons under the bar

## Context
The menu button on the navigation bar pushed `pages/MenuPage.qml`: a page of text rows —
New tab, Bookmark, Share, then Tabs, Move tab to group, Bookmarks, History, Settings. A page
is a sideways transition and a back gesture for one tap's worth of choosing, and a column of
words is slow to find things in for a hand that already knows where they are. What was
asked for instead is an overlay like the list of closed tabs (0018), holding icons of three
kinds: the tabs (a new tab); the page in front (search on it, bookmark it, share it, ask for
its desktop version); and the browser (bookmarks, history, downloads, settings).

## Decision
`components/BrowserMenu.qml` is a Silica `DockedPanel`, docked to the bottom and modal —
the same kind of sheet as the closed tabs, so the two come and go alike: it slides up from
under the navigation bar, where its button is, and a tap outside it puts it away. Nothing
is pushed for it, and `MenuPage.qml` is gone. It holds three rows under a section header
each — *Tabs*, *This page*, *Browser* — of `components/MenuButton.qml`, an icon with its
name written under it, four to a row. An entry that is a switch (the page bookmarked, the
page in its desktop version) is drawn in the highlight colour while it is on. Each entry
does what it says and puts the sheet away; the browser's own pages are pushed over the
browsing page, which stays where it was under them.

The icons are the theme's, by the names sailfish-browser gives the same entries
(`apps/browser/qml/pages/components/PopUpMenuItem.qml`): `icon-m-tab-new`,
`icon-m-search-on-page`, `icon-m-favorite` and `-selected`, `icon-m-share`,
`icon-m-computer`, `icon-m-history`, `icon-m-downloads`, `icon-m-setting`. The host tests
draw nothing, so a name that is not in the device's theme would pass them; names a Jolla
application ships with are the best evidence available off the device.

Opening the menu **ends editing the address**: the sheet comes up where the keyboard would
be sitting over it. **Another page in front** — the cover's new tab, say — puts the sheet
away, since what it offers is for the page in front.

**Search on page** is Gecko's own find, driven the way sailfish-browser drives it
(`apps/browser/qml/pages/components/ToolBar.qml`, `apps/shared/WebView.qml`): the page's
script, `jsscripts/embedhelper.js` in embedlite-components, answers `embedui:find` with a
`Finder` of its own, marks the match and scrolls to it, and replies on `embed:find` with
nsITypeAheadFind's result. The words, the request's shape and how to read the reply are
`EngineMessages`' (`findMessage`, `findResultMessage`, `findRequest()`, `findFound()`), so
QML carries none of them. Every view registers for the reply as it is made
(`addMessageListener`, which qtmozembed holds until the view is initialised). The field is
`components/FindBar.qml`, laid over the navigation bar — which stays whole under it however
the page scrolls — with the previous and next match and a close button beside it. Enter
searches from the top of the page and puts the keyboard away so the match can be seen; the
arrows step on from there, round the ends of the page; the field turns to the error colour
when there is nothing to find. Closing it, or another page coming to the front, sends the
empty search that makes the page take its highlight away — to the page that was searched,
whichever is in front by then.

**Desktop version** is set on the view, `desktopMode`, as sailfish-browser sets it on its
own; qtmozembed hands it to the engine, which loads the page again as a desktop browser
would be sent it. The switch is on while the page in front is in its desktop version,
whether it got there by the switch or by the setting every page starts from, and the choice
is that view's alone. The view keeps it for as long as it lives: one given up past the limit
of loaded pages (0016) comes back as Settings says. sailfish-browser keeps it with the tab
in its database; here that would be a schema change for a choice most pages are asked once.

**Downloads** is the browser's own list of them (0022).

What the menu page had and the sheet does not:

* **Tabs**, the way into the grid without the gesture (0009). The drag on the bar has
  worked on device since the build that needed the fallback, and the sheet holds what was
  asked for; if the gesture is ever the problem again, this is where the way back in goes.
* **Move tab to group**. A tab changes group by being carried onto one in the grid's strip
  (0015), which shows where it is going; making a group with a tab in it is now two steps —
  the group from the list of groups, then the tab carried there.

## Consequences
The browsing page holds an instance of each component and nothing of their insides, and
stays under the 600 lines 0010 allows it: 599. The next thing it needs has to come out of
it first, as 0010 says.

`tests/tst_qmlload.cpp` drives the sheet, the find bar and their pages by `objectName`
(`browserMenu`, `findInPage`); the WebView stub records what is sent to the page and which
names it listens for, and a test raises the page's reply. Whether the icons are the right
ones, whether a match is marked and scrolled to, and whether a page comes back in its
desktop version are device checks (`docs/TESTING.md`).
