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
under the navigation bar, where its button is, and a tap outside it puts it away, as does
pulling it back down. Nothing is pushed for it, and `MenuPage.qml` is gone. It holds two
rows under a section header each — *This page*, *Browser* — of
`components/MenuButton.qml`, an icon with its name written under it, four to a row. An
entry that is a switch (the page bookmarked, the page in its desktop version) is drawn in
the highlight colour while it is on. Each entry does what it says and puts the sheet away;
the browser's own pages are pushed over the browsing page, which stays where it was under
them.

**The pull is the sheet's own.** On device the first build's sheet could be put away by a
tap outside it but not by pulling it down from its icons, while the list of closed tabs
goes down with a pull begun on its rows. `DockedPanel` is meant to take either: its
content sits in a `MouseArea` that filters its children's presses and drags the panel. Why
it takes the pull over a row of the closed tabs' list and not over one of these icons —
both Silica `BackgroundItem`s, the one in a `SilicaListView` and the other not — could not
be settled off the device; Qt 5.6's own delivery, read for it, would give it both. So the
icons sit in a `SilicaFlickable` of their own, as the closed tabs sit in a list, and a
pull down on them is that flickable's overscroll: the same stealing of a drag from a
pressed Silica button that every list on the platform does to scroll. The sheet follows
it, as the tab grid is pulled back to the page (0010): the flickable is moved up by as
much as it draws the icons down, and the sheet down by the pull, so the icons stay under
the finger. A flickable draws a pull past its bounds at half the finger's way, as a rubber
band gives, and the sheet goes twice that, which is the finger's way: the closed tabs'
sheet goes with the finger, and this one does too. Released more than `DockedPanel`'s own
distance down — a third of the sheet, up to about a large item's height — it is put away,
and `DockedPanel` takes it from there; released short of that, it springs back up with the
flickable. Where `DockedPanel`'s own drag does take a pull, it moves the sheet as it would
anyway, and closes it the same way.

The icons are the theme's, by the names sailfish-browser gives the same entries in
`apps/browser/qml/pages/components/PopUpMenuItem.qml`:
`icon-m-search-on-page`, `icon-m-share`, `icon-m-computer`, `icon-m-favorite-selected`
(Bookmarks), `icon-m-history`, `icon-m-downloads`, `icon-m-setting`; and in
`PopUpMenuFooter.qml` beside it, `icon-m-favorite` and `-selected` for bookmarking the
page. The host tests draw nothing, so a name that is not in the device's theme would pass
them; names a Jolla application ships with are the best evidence available off the device.

Opening the menu **ends editing the address**: the sheet comes up where the keyboard would
be sitting over it. **Another page in front** puts the sheet away, since what it offers is
for the page in front; the cover's quick action puts it away before anything else (0029).

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
would be sent it. The switch is on while the page in front is in its desktop version, and
the choice is that view's alone: it is the only way to ask for one, Settings' *Request
desktop sites* for every page having been taken out (0028). The view keeps it for as long
as it lives: one given up past the limit of loaded pages (0016) comes back in its phone
version. sailfish-browser keeps it on its tab in
memory (`Tab::m_desktopMode`, `apps/storage/tab.h`) and hands it to the page it makes again
(`apps/qtmozembed/declarativewebpage.cpp`); here that would be a role on the tab model and
the browsing page handing it back, for a page asked for its desktop version and then
unloaded, which is rare.

**Downloads** is the browser's own list of them (0022).

What the menu page had and the sheet does not:

* **New tab**, which the first sheet had alone in a row of its own, *Tabs*. The plus in
  the grid's foot opens one (0010), and the cover's search opens the address bar for one
  (0027, 0029): a row for one entry was a third of the sheet for what two places give.
* **Tabs**, the way into the grid without the gesture (0009). The drag on the bar has
  worked on device since the build that needed the fallback, and the sheet holds what was
  asked for; if the gesture is ever the problem again, this is where the way back in goes.
* **Move tab to group**. A tab changes group by being carried onto one in the grid's strip
  (0015), which shows where it is going; making a group with a tab in it is now two steps —
  the group from the list of groups, then the tab carried there.

## Consequences
The browsing page holds an instance of each component and nothing of their insides, and
stays under the 600 lines 0010 allows it: 599 when the sheet came, 580 once the bar read
its load progress and the connection's verdict from the view itself (0009) and the page's
`showTab()` went with the search page (0015), and 599 again by the time tracking
protection, the reader view and the media controls had come. The deck's state and gestures
then went to a component of their own, as 0010 said they would; with the omnibar's pane
in it (0027) the page was 592 lines, and with what the cover's quick action asks of it
(0029), 599. The next thing it needs has to come out of it first.

`tests/tst_qmlload.cpp` drives the sheet, the find bar and their pages by `objectName`
(`browserMenu`, `findBar`); the WebView stub records what is sent to the page and which
names it listens for, and a test raises the page's reply. `menuSheetUnderAFinger` pulls
the sheet in a real window, short of the distance and past it; the stub `DockedPanel` lies
along its edge while open for it. Its icons take no presses, so it proves the sheet's own
pull, and whether the flickable takes the pull from a pressed Silica icon on the phone is
a device check. Whether the icons are the right ones, whether a match is marked and
scrolled to, and whether a page comes back in its desktop version are device checks
(`docs/TESTING.md`).
