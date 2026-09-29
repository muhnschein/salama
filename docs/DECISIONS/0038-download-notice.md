# 0038 — The downloads happening are said over the page

## Context
Downloads are the browser's, listed by `pages/DownloadsPage.qml` from `DownloadModel`
(0022, 0025). A download under way is visible in two places only: as a ring round
Downloads in the menu sheet (0021), which must be opened, and as a ring on the cover
with the percentage in it (0037), which must have the application put away. Nowhere on
the browsing page does a download say anything, and nothing at all says that one has
begun, finished or failed while the reader is looking at the page.

A first build of this put a strip over the page that followed one download at a time. On
a phone it read badly: a second download took the first's place, a first finishing took
the strip away while a second was still coming, its line along the foot was taken for
the page's own load progress, drawn along the top of the bar in the same place, and it
was laid over the page, covering buttons and media controls along the page's foot.

## Decision
`components/DownloadBanner.qml` is a strip along the top of the navigation bar, the
colour of the bar, so the two read as one piece of chrome. It is about the downloads
**together**, never one of them in turn:

* a circular `ProgressCircle`, filled as far as the downloads under way have come
  together (`DownloadModel.runningProgress`, the mean of their percentages, as the
  menu's ring reads it), with the percentage in it;
* how many are still coming (`DownloadModel.runningCount`) — or, with one coming, its
  name (`DownloadModel.runningName`);
* how long they will take at the pace they have been going
  (`DownloadModel.etaSeconds`), said as *less than a minute*, *n minutes left* or
  *n hours left*;
* how many have finished while these have been coming (`DownloadModel.finishedCount`),
  with a check, or a warning when any of those failed (`DownloadModel.failedCount`), in
  which case the ring is drawn in the error colour.

It comes up while anything is coming and goes when nothing is: a second download
starting does not take the first's place, and a first finishing does not take the strip
away while a second is still coming. A tap opens the list of them.

**The page ends where the strip begins.** `pages/BrowserPage.qml` keeps the strip's
height in `downloadInset` and takes it off the engine's view — `viewArea.height` is
`viewHeight - cutoutInset - downloadInset` — so the engine lays the page out only above
it, and nothing of the page is covered. The strip's height is fixed, so the view is
resized once as it comes and once as it goes, not as figures move.

`DownloadModel` grew `finishedCount` and `failedCount`, and a `finished` stamp per row
(not written down: like progress, it is only true while this process runs). Recent is
the batch: a download counts if it ended while the ones still coming have been coming,
measured from the earliest of them. Nothing coming, the counts are 0, and the strip is
away. A batch is what the strip is for, not the list's fifty; and it is not a clock,
which a binding could not hear run out. Cancelled, and one still coming, do not count as
finished. Both counts stand beside `runningCount`/`runningProgress`, and are announced
by `finishedChanged`.

**The estimate of the time left** is the same story of the batch, from a pace. Working
from sizes would be surer, but the engine reports a percentage alone; a pace is two
percentages and the time between them. Each download keeps a smoothed pace (`rate`, in
percent per second) from progress reports a second apart, and the strip's figure is the
time of the one with the furthest to go, so the batch is done when it is. `-1` while
nothing can be said: nothing is coming, or nothing has moved.

The list of downloads replaces its line along the foot with a ring beside the file's
name, `ProgressCircle` again: the line in the same place as the page's load progress is
what this decision is against.

**A row's menu does what a download needs.** `components/DownloadDelegate.qml` offers:

* *Pause* and *Resume* — the platform engine has no pause. Its cancel keeps the partial
  file, so the same download can start again in the same session, and its retry starts
  it again from there. So pausing is the engine's cancel, and resuming its retry
  (`EmbedliteDownloadManager.js` answers `cancelDownload` and `retryDownload` on
  `embedui:download`, `{"msg", "id"}`). The row reads *Paused at n%* — a status of the
  model's own, since the engine sends none — and the engine's echo of its own cancel is
  not allowed to untell it.
* *Cancel* — the engine's cancel, and the row stays to say *Cancelled*. A cancelled
  download is not removed from the list; only clearing the list takes it.
* *Delete file* — removes the saved file from its folder (`DownloadModel.deleteFile`),
  when the download has arrived and the file is still there. The row stays; without the
  file there is nothing left to open.
* *Remove from list*, as before — the record alone, the file untouched.

Each command goes to the engine the way NotificationPermissions' do: the model emits
`engineRequest(topic, payload)`, and the browsing page, the only one with the engine,
sends it with `WebEngine.notifyObservers`.

## Consequences
The strings are under `DownloadBanner` and `DownloadDelegate`: the percentage `%1%`;
*Less than a minute left*, `%1 minute(s) left`, `%1 hour(s) left`; *Pause*, *Resume*,
*Cancel*, *Delete file*, *Paused at %1%*; and the older *Downloading, %1%*, *Failed*,
*Cancelled*, *Remove from list*.

The strip lives on the browsing page alone; a page pushed over it — the list of
downloads, a settings page — covers it, and a download that ends while one is up is
found in the list.

Because the strip goes when nothing is coming, a lone download that fails leaves
nothing over the page once it has ended: the list says so, and the menu's Downloads is
where it is found.

A paused download is not counted among those coming: the menu's ring and the strip say
the browser is idle, and the list says what it is waiting on. A pause is not written
into the status for good: read back after a restart, a paused download is failed as the
rest of a run that never finished is.

`tests/tst_downloadmodel.cpp` drives the counts (`countsFinished`), the commands
(`pauseResumeCancel`), the estimate (`estimatesTimeLeft`) and the file (`deletesFile`);
`tests/tst_qmlload.cpp` drives the strip by `objectName` (`downloadNotice`), including
the page giving it the room and the one name giving way to the count, and the menu
(`downloadListActions`), including the words sent to the engine. The strip's look, the
pace's truthfulness, the theme icons, whether the engine's retry truly resumes, and
whether `Qt.openUrlExternally` and file removal reach the folder from inside Sailjail
are device checks (`docs/TESTING.md`).