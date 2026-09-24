# 0022 — Downloads are listed in the browser

## Context
The menu has a way to the downloads (0021). The platform's list of them is Settings >
Transfers, and sailfish-browser opens it with `com.jolla.settings.ui.showTransfers` over
D-Bus. Sailjail lets **only the platform's own applications' profiles** make that call —
`permissions/sailfish-browser.profile` in sailjail-permissions names it, and
`jolla-contacts.profile` allows every method of that interface — and no permission a
Harbour application may ask for grants it. The list would not hold this browser's
downloads in any case: its entries are made by sailfish-browser itself
(`apps/core/downloadmanager.cpp`), from the engine's `embed:download` notifications, and
sailfish-components-webview makes none for the other applications that embed the engine.
SCOPE.md's "Downloads via platform transfer UI" had no list behind it.

## Decision
`DownloadModel` (`src/downloads/`) listens to the same notifications, which
embedlite-components sends from `jscomps/EmbedliteDownloadManager.js`: `dl-start` with the
file's name, where it came from, where it goes, its type and size; `dl-progress`;
`dl-done`, `dl-fail` and `dl-cancel`. The browsing page subscribes to the topic beside
PageActivity's and hands the model what arrives on it. The model keeps the last fifty
downloads, newest first, in a `download` table (schema 7), each with the way it ended. The
engine forgets every download when it starts, so one still running when the application
stopped can never finish: it is marked failed when the list is next read. Progress is not
written down; it is only true while the engine is sending it.

`pages/DownloadsPage.qml` lists them — the file's name, and under it how far along it is,
how it ended, or the site it came from, with a line along the foot of one still coming. A
tap on a finished download opens the file with `Qt.openUrlExternally`, in whatever the
platform opens that kind of file with. A row's menu forgets it and the pulley forgets them
all; the files are left where they are, since the list is not the only thing that knows
them.

## Consequences
There is no cancelling or retrying from the list. The engine takes both
(`cancelDownload` and `retryDownload` on `embedui:download`), but saying so goes through
`WebEngine`, which `docs/ARCHITECTURE.md` keeps to the browsing page and Settings
(`tst_qmlstatic::webViewImportOnlyInBrowserPage` holds it there); it is a later change,
not a refusal.

The files land in `~/Downloads/Salama` (0025), which the `Downloads` permission already
opens (`docs/HARBOUR.md`). Whether `Qt.openUrlExternally` hands a file on from inside
Sailjail is a device check (`docs/TESTING.md`); if it does not, a tap on a finished download
is the thing to change.
