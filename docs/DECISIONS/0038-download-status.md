# 0038 — Downloads say how they are going where you are, and can be paused

## Context
Issue #23. A download started from a page said nothing on the page. Its progress was in
Menu > Downloads, and a ring round the menu's Downloads icon (0021), neither of which
anyone sees without going to look; a novice need never find either. The list itself drew
progress as a thin line along a row's foot, said "Failed" and "Cancelled" in the same grey
as everything else, and could neither pause, resume nor retry a download, open the folder
the files are in, nor delete a file -- only forget a row (0022).

What the engine allows, from embedlite-components' `jscomps/EmbedliteDownloadManager.js`,
on topic `embedui:download`:

- `cancelDownload {id}` calls Gecko's `Download.cancel()`. For a download from a page
  (`DownloadLegacy`) Gecko keeps the partial data, so this is the pause Firefox's own
  Pause button is.
- `retryDownload {id}` calls `Download.start()`, which goes on from the partial data where
  the server allows it and starts over where it does not -- Firefox's Resume and Retry.
- `addDownload {from, to}` makes a new download. The engine forgets every download when it
  starts, so this is the only way to fetch one of an earlier run again.

There is no cancel that is not also a pause, and the engine says `dl-cancel` either way.

## Decision
**On the browsing page**, `components/DownloadBanner.qml` comes up just above the bar as a
download starts, and speaks for this run's downloads that have not arrived -- coming,
paused or failed -- which `DownloadModel`'s *tray* counts:

- One: its name; how much of how much and the percentage (or the percentage alone, with
  the size unknown), "Paused · 40%" or "Failed" in the error colour; and Silica's ring at
  its start, as the menu and the cover draw it.
- More: "3 downloads · 42%" -- the ring and the figure the mean of their percentages, as
  the menu's ring is, failed ones left out -- and no line under it: not their names.
- When one arrives it says "*name* · Downloaded" for four seconds.

The banner has no button. A tap on it, whatever it says, opens Menu > Downloads, never a
file: what is done to a download is done there, where each has its own. A swipe sideways takes the banner away: the rows on
it are marked dismissed, not stored, and come back when they change state -- paused,
failed, started again -- or a new download starts; progress alone brings nothing back. It
is out of the way while the address is edited, a word looked for, or the grid is out, and
it goes once there is nothing to say. It does not expand into a list in place: the list
of downloads is one tap away, and one place to act on several is enough.

*Revised with 0046.* The banner is no longer a card over the foot of the page. It is the
bar every banner on the browsing page is (`components/BarBanner.qml`), the same as the one
that says where a link opened behind the page went: the width of the screen, lying on the
navigation bar, on the sheets' ground with its edge in the highlight colour, the ring in a
slot at its start, the words, and at its end **Show**, which opens Downloads as a tap on the
bar does — the one thing to do from it, named. Swiping it away is as before. The two lie one
above the other (`BarBanners.qml`), and the page ends where they begin: the browsing page
takes their height off the page's (`BrowserPage.viewHeight`), so the view is resized once as
a banner comes and once as it goes, never frame by frame, as the bar's own slimming does.

**In the list**, each row's foot line is a ring at its start instead, with its action in
the middle -- pause while coming, play while paused, retry once failed, the ring dimmed
and red respectively -- and the icon of its kind of file once it has arrived. The line
under the name says "3.1 MB of 7.4 MB · 42%", "Paused · 42%", "Failed" in the error colour,
"Stopped" for one paused in an earlier run, the size and the site once arrived, or "File
not found" when the file has gone since (checked as the page opens). Its menu: Open; Pause,
or Resume / Retry, or Download again for one of an earlier run, which the engine has
forgotten -- `addDownload` from where it came from into where it was going, its row giving
way to the engine's new one; Delete file, after Silica's remorse timer; Remove from list.
The pulley: Clear finished.

There is no Open folder: `Qt.openUrlExternally` on a folder opens nothing on the device
from inside Sailjail. No Copy link, and no Clear list: the list is cleared with the
history, and one row at a time.

Removing a row pauses a download still coming first, so that nothing goes on arriving
that no list knows of; so does `clear()`, which clearing the history when the browser
closes calls.

**Delete file** deletes only a file that has arrived and lies, links and `..` followed,
under the downloads folder's parent, `~/Downloads` -- where the engine saves when its own
folder is gone (0025), and what Sailjail's `Downloads` permission covers. A file already
gone takes only its row.

**Telling the engine** goes through `WebEngine`, which `docs/ARCHITECTURE.md` keeps to the
browsing page (`tst_qmlstatic::webViewImportOnlyInBrowserPage`). The model raises
`engineRequest(topic, data)`, and the browsing page hands it to
`WebEngine.notifyObservers()`, as it hands the engine's notifications to the model.

## Consequences
A download is visible where it was started from, and can be paused, resumed, retried,
fetched again, opened and deleted from the list.

Whether a resume goes on from where it stopped is the server's: one without range
requests starts over, and the ring with it.

The banner sat over the foot of the page while it showed; since 0046 the page ends above
it instead, and is laid out again once as it comes and once as it goes. A swipe puts it
away.

There is still no system notification: the banner, the cover and the menu ring speak while
the app is in use or on the home screen, and a notification would be a decision of its own.
