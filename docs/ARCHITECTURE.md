# Architecture

## Module boundaries

| Layer | Location | Owns | Knows about |
|---|---|---|---|
| Engine | platform `Sailfish.WebView` | rendering, navigation history, cookies, dialogs, pickers, downloads | nothing of ours |
| UI | `qml/` | pages, components, cover | the `harbour.tuuli` singletons |
| Core | `src/` | tabs, history, bookmarks, settings, engine-facing strings | SQLite, QSettings |

`Sailfish.WebView` is imported in `qml/pages/BrowserPage.qml` only; a device without
the engine package fails to open that page, not the application. `Sailfish.WebEngine`
is imported there and in `SettingsPage.qml` (data clearing). `tests/tst_qmlstatic.cpp`
enforces both.

The core is one process-wide `Tuuli::Core` (`src/Core.h`) that owns:

- `Storage` — the single SQLite file and its schema.
- `TabModel` + `TabPersistence` — open tabs, the active tab, private flag.
- `HistoryModel` — visited pages, search, pruning.
- `BookmarkModel` — bookmarks and "is the active page bookmarked".
- `Settings` — home page, search engine, desktop mode, address-bar heuristics.
- `EngineMessages` — the only place engine-specific strings live.

`registerQmlTypes()` exposes each as a QML singleton under `harbour.tuuli 1.0`.

## Data flow

1. The engine reports `url`, `title` and load state on a `WebView`.
2. `BrowserPage` forwards them to `TabModel.updateUrl/updateTitle/updateFavicon`, and
   grabs a page preview into the path `TabModel.thumbnailPath()` hands out — on load
   completion, when the grid opens, and as the application leaves the screen — reporting
   it back through `updateThumbnail()`.
3. `TabModel` updates its row, persists non-private tabs, and emits `visited`,
   `titleUpdated`, `faviconUpdated` for non-private tabs only.
4. `Core` wires those signals to `HistoryModel` and `BookmarkModel`. Private tabs
   therefore never reach history or disk; the engine's `privateMode` keeps cookies out.
5. `TabModel.activeTabDataChanged` feeds the address bar and
   `BookmarkModel.activeUrl`. The cover reads `count` and the rows themselves: it says
   how many tabs are open over a monochrome field of their previews, most recently in
   front first (`TabModel.recentThumbnails`, ordered by each tab's `last_active` stamp),
   and names no page (`DECISIONS/0014-cover-is-the-tab-count.md`).

Views: one `WebView` per tab that has been shown this session, created lazily by a
`Loader` (see `DECISIONS/0003-one-webview-per-tab.md`). Restored tabs cost nothing
until activated. Favicons come from a page script with `/favicon.ico` as fallback
(`DECISIONS/0005-favicons.md`), and a page's `theme-color` from another one
(`DECISIONS/0013-screen-cutout.md`); both are asked of the page because the `WebView`
Harbour allows carries neither. Tab previews are scene-graph grabs written to the cache
directory (`DECISIONS/0008-tab-previews.md`); a tab that has not been displayed this
session has none, and shows a placeholder in the grid.

The browsing page carries the address: a label until tapped, a field in place after.
The navigation bar along the bottom is also the surface the tab grid is dragged from:
the page and the grid are one deck two screens tall, the grid below the page, and the
grid's own overscroll drops the page back onto it. Nothing is pushed onto the page
stack for it (`DECISIONS/0009-navigation-bar-gesture.md`,
`DECISIONS/0010-tab-grid-deck.md`).

The bar shows `Settings.displayAddress(url)` -- the host alone -- until it is tapped, and
draws a red open padlock when the engine reports a broken TLS connection for an https page
(`DECISIONS/0011-address-and-security.md`). The bar follows the engine's own chrome gesture off the
bottom of the page while a page is scrolled down, so the foot of a page can be reached
under it.

Typed text goes through `Settings.urlForInput`: a URL with a known scheme is used as
is, a host-like token gets `https://` (`http://` for localhost and IP addresses),
anything else becomes a search with the selected engine.

## Storage

Location: `QStandardPaths::AppDataLocation` (Sailjail: `~/.local/share/<org>/<app>`),
file `tuuli.sqlite`. Settings: `AppConfigLocation/tuuli.conf` (INI). Tab previews are
PNG files in `CacheLocation`, named per capture and removed with the tab. Nothing else
is written. Schema version is `PRAGMA user_version` (`Storage::SchemaVersion`, currently
2); a newer database than the build refuses to open rather than corrupt. Migration asks
the table for its columns rather than trusting the version number, so a database from
either schema converges on the same shape.

```
tab              tab_id PK, position, url, title, favicon, thumbnail, last_active
browser_history  id PK, url UNIQUE, title, visited_count, date (ms since epoch)
bookmark         id PK, url, title, favicon, position, created (s since epoch)
setting          name PK, value          -- activeTabId
```

History is capped at 2000 rows (pruned on open) and the model shows the newest 500.
Queries run on the UI thread; sizes are bounded, so no worker thread
(`DECISIONS/0004-sqlite-storage.md`).
