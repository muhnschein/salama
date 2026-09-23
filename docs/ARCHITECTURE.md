# Architecture

## Module boundaries

| Layer | Location | Owns | Knows about |
|---|---|---|---|
| Engine | platform `Sailfish.WebView` | rendering, navigation history, cookies, dialogs, pickers, downloads | nothing of ours |
| UI | `qml/` | pages, components, cover | the `harbour.salama` singletons |
| Core | `src/` | tabs, history, bookmarks, settings, engine-facing strings | SQLite, QSettings |

`Sailfish.WebView` is imported in `qml/pages/BrowserPage.qml` only; a device without
the engine package fails to open that page, not the application. `Sailfish.WebEngine`
is imported there and in `SettingsPage.qml` (data clearing). `tests/tst_qmlstatic.cpp`
enforces both.

The core is one process-wide `Salama::Core` (`src/Core.h`) that owns:

- `Storage` — the single SQLite file and its schema.
- `TabModel` + `TabPersistence` — open tabs, the active tab, tab groups, the current
  group, and which tabs keep their page loaded. `TabModel` owns
  three views of itself for QML: `GroupTabModel` (`GroupTabs`), the current group's
  tabs, which the grid shows; `TabGroupModel` (`TabGroups`), the groups, which the strip
  along the grid's foot shows and the group actions are reached through
  (`DECISIONS/0015-tab-groups.md`); and `ClosedTabModel`
  (`ClosedTabs`), the tabs closed lately (`0018-recently-closed.md`).
- `TabSearchModel` (`TabSearch`) — the open tabs matching a term, group by group.
- `HistoryModel` — visited pages, search, pruning.
- `BookmarkModel` — bookmarks and "is the active page bookmarked".
- `DownloadModel` — the downloads, read from the engine's own `embed:download`
  notifications, because the platform's list of transfers is closed to a Harbour
  application and would not hold a `Sailfish.WebView` application's downloads anyway.
- `Settings` — home page, search engine, desktop mode, cover style, address-bar heuristics.
- `EngineMessages` — the engine-specific strings QML hands to the engine.
- `PageActivity` — what the engine says is playing, read from its own observer topics,
  and so when the loaded pages are put to sleep out of sight
  (`DECISIONS/0020-pages-sleep-out-of-sight.md`).

`registerQmlTypes()` exposes each as a QML singleton under `harbour.salama 1.0`.

## Data flow

1. The engine reports `url`, `title` and load state on a `WebView`.
2. `BrowserPage` forwards them to `TabModel.updateUrl/updateTitle/updateFavicon`, and
   grabs a page preview into the path `TabModel.thumbnailPath()` hands out — on load
   completion, when the grid opens, and as the application leaves the screen — reporting
   it back through `updateThumbnail()`.
3. `TabModel` updates its row, persists the tab, and emits `visited`, `titleUpdated`,
   `faviconUpdated`.
4. `Core` wires those signals to `HistoryModel` and `BookmarkModel`. There are no
   private tabs (`DECISIONS/0019-no-private-tabs.md`).
5. `TabModel.activeTabDataChanged` feeds the address bar and
   `BookmarkModel.activeUrl`. The cover reads `count` and the rows themselves: it says
   how many tabs are open over a monochrome field of their previews, most recently in
   front first (`TabModel.recentThumbnails`, ordered by each tab's `last_active` stamp),
   and names no page (`DECISIONS/0014-cover-is-the-tab-count.md`).

Views: one `WebView` per tab that has been shown this session and is among the
`Settings.liveTabLimit` most recently in front, created lazily by a `Loader` over
`TabModel` -- every group's tabs, so a tab changing group keeps its view (see
`DECISIONS/0003-one-webview-per-tab.md`, `0016-five-live-pages.md`). Restored tabs
cost nothing until activated; a tab beyond the limit reloads when it is next in front. Favicons come from a page script with `/favicon.ico` as fallback
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

The bar shows `Settings.displayAddress(url)` -- the host alone -- until it is tapped,
and draws a red open padlock when the engine reports a broken TLS connection for an
https page (`DECISIONS/0011-address-and-security.md`). The bar slims to the host on the
engine's own chrome gesture while a page is scrolled down, the page ending above it
either way, and a tap on the slim bar brings the whole bar back
(`DECISIONS/0009-navigation-bar-gesture.md`).

Typed text goes through `Settings.urlForInput`: a URL with a known scheme is used as
is, a host-like token gets `https://` (`http://` for localhost and IP addresses),
anything else becomes a search with the selected engine.

## Storage

Location: `QStandardPaths::AppDataLocation` (Sailjail: `~/.local/share/<org>/<app>`),
file `salama.sqlite`. Settings: `AppConfigLocation/salama.conf` (INI). Tab previews are
PNG files in `CacheLocation`, named per capture and removed with the tab. Nothing else
is written. Schema version is `PRAGMA user_version` (`Storage::SchemaVersion`, currently
7); a newer database than the build refuses to open rather than corrupt. Migration asks
the table for its columns rather than trusting the version number, so a database from
any earlier schema converges on the same shape; a column that a later schema dropped
takes its table through a rebuild (`DECISIONS/0019-no-private-tabs.md`).

```
tab              tab_id PK, position, url, title, favicon, thumbnail, last_active, group_id
tab_group        group_id PK, name, position
closed_tab       id PK, url, title, favicon, closed (ms since epoch)
browser_history  id PK, url UNIQUE, title, visited_count, date (ms since epoch)
bookmark         id PK, url, title, favicon, position, created (s since epoch)
download         id PK, name, url, path, mime, size, status, started (ms since epoch)
setting          name PK, value          -- activeTabId, currentGroupId
```

History is capped at 2000 rows (pruned on open) and the model shows the newest 500.
Queries run on the UI thread; sizes are bounded, so no worker thread
(`DECISIONS/0004-sqlite-storage.md`).
