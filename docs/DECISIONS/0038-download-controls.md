# 0038 — Downloads are seen as they come, and can be stopped, retried and deleted

## Context
A download started from a page said nothing on the page: the only sign of it was the ring
round Downloads in the menu sheet (0021), which is not on the screen until the sheet is
brought up, and which a reader new to the browser would not know to look for. In the list
(0022) a download still coming was a line along its row's foot, one that failed or was
cancelled a word under its name, and nothing could be done with any of them but open the
file of one that arrived or forget the row: there was no stopping one, no fetching one
again, no getting to the folder they are saved in, and no deleting a file (issue #23).

What the engine offers is embedlite-components' `jscomps/EmbedliteDownloadManager.js`,
which observes `embedui:download` beside the `embed:download` it sends:

* `cancelDownload {id}` calls the download's `cancel()`. Gecko keeps nothing of the file
  unless `tryToKeepPartialData` is set, and nothing a `Sailfish.WebView` application can
  send sets it.
* `retryDownload {id}` calls the download's `start()`: from the start, as the file was
  not kept. The engine says `dl-start` again for the same id.
* `addDownload {from, to}` creates a download from an address to a file and starts it,
  under a new id.

The engine forgets every download when it starts (0022), so the ids it knows are this
run's alone. There is no pause.

## Decision
**On the browsing page**, `components/DownloadBar.qml` comes up just above the navigation
bar as a download starts, on the menu sheet's ground with rounded corners, and names the
newest download with where it stands: how far along -- or, with more than one coming, how
many and how far they have come together -- with a close that puts the bar away and leaves
the download going; arrived, "Downloaded" with Open for five seconds; failed, "Failed" with
Retry for ten. Stopped on purpose, it says nothing. A tap anywhere else on it is the list.
It is out of the way while the menu sheet, the address being edited, the find bar or the
grid is over the foot of the page, and back after. While anything is coming, the
navigation bar's menu button wears the ring the menu's Downloads wears, so a download is
seen to be under way after the bar has gone. While the browser is out of sight
(`PageActivity.background`), a download that arrives or fails is a Nemo notification
instead (`components/DownloadNotifier.qml`, in the root window): a tap on one that arrived
opens the file, on one that failed the list, with the browser brought forward.

**In the list**, each row leads with `components/DownloadIndicator.qml`, the same circle
the bar has: still coming, Silica's ring as the menu draws it, round the stop; failed or
stopped, the arrow it is fetched again by, in the error colour for a failure; arrived, the
theme's icon for the kind of file, dimmed once the file has gone. The line under the name
says how far along and of how much, "Failed, tap to retry" in the error colour, "Stopped,
tap to retry", the size and the site, or "Moved or deleted". A tap opens a file that is
there and fetches again one that failed or was stopped; the circle stops one coming. The
row's menu has Stop, Retry, Open, Copy link, Delete file -- the file and the row, after a
remorse -- and Remove from list, each where it applies. The pulley has Open folder and
Clear list; clearing leaves what is still coming, which would otherwise go on with nothing
left to stop it by (`clearEnded()`). Clearing the history when the browser closes still
takes every row (`clear()`): the engine stops with the browser, and nothing is left coming.

**The words are Stop and Retry**, not Pause and Resume: the engine keeps nothing of a file
it stops, and a Resume that starts from nothing would say what does not happen. A download
fetched again is said to start from nothing, and its row's progress goes back to 0.

**What the engine is told** comes from `DownloadModel`, which knows each row's engine id:
`stop()`, `retry()` and `deleteFile()` emit `engineRequest(topic, data)`, and the browsing
page, which alone has `WebEngine` (`docs/ARCHITECTURE.md`), sends it on with
`notifyObservers`. A row of this run is retried by its id. A row from an earlier run is
fetched anew with `addDownload` from its address to its file, and marked as waiting: the
`dl-start` that comes for a new id with that row's path is that row, started again, not a
new one. One whose address means nothing without the page that made it -- `blob:`,
`data:` -- or that has nowhere to go cannot be fetched anew, and the list does not offer
it (`DownloadModel::canRetry()`). The model emits `downloadStarted` and `downloadEnded` for
the bar and the notifications, says whether a file is there (`fileExists`, asked of the
disk; `refresh()` as the list comes up), and names the theme's icon for a type, by its
MIME type and else its extension (`iconFor()`), as the platform's file manager draws one.

## Consequences
Opening the folder is `Qt.openUrlExternally` on its `file://` URL, which the platform hands
to whatever opens folders; that it reaches the file manager from inside Sailjail is a
device check (`docs/TESTING.md`), as opening a file was (0022).

A row fetched anew with `addDownload` is fetched by its address alone, without the page's
request: a download that needed a form posted, or a session the site has since ended,
fails again, and says so.

Deleting a file deletes what is at the row's path, if that is a file; a folder there is
left alone. The engine stops one still coming first, and takes away what it had written.

### The size of the navigation bar
`qml/components/NavigationBar.qml` stood at 399 lines, and the ring round the menu button
takes it past the 400 SCOPE.md §7 allows a QML file. It is waived here rather than split:

qml-size-waiver: qml/components/NavigationBar.qml

The ring is one `DownloadRing` placed on the menu button, which is the bar's; the bar's
length is its one gesture area and the regions that area hands presses to, and splitting
those apart would part the code that decides what a press means from the things it can
mean. The ring's look is `DownloadRing`'s, shared with the menu's Downloads and each
download's circle, so the bar carries where it goes and nothing of how it is drawn.
