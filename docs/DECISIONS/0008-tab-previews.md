# 0008 — Tab previews are scene-graph grabs in the cache directory

## Context
The tab grid shows a picture of each page. The engine offers no thumbnail API:
`qtmozembed` exposes none, and sailfish-browser's own capture path is private to it.

## Decision
`BrowserPage` calls `QQuickItem::grabToImage` on the active `WebView` when a load
finishes and again when the grid is opened, saving to a path `TabModel.thumbnailPath()
hands out. The model owns the files: a fresh name per capture (so a new image is never
hidden behind a cached one), the previous file removed when the new path is reported
back, and every file removed when its tab closes. Files live in `CacheLocation`, and
`discardThumbnail()` refuses to delete anything outside it. A private tab is given no
path at all, so none of its pages reach the disk.

## Consequences
A grab needs a rendered item, so a tab that has not been displayed this session has no
preview and the grid shows a placeholder; restored tabs fill in as they are visited.
Losing the cache directory costs placeholders, not data. Whether the engine's
hardware-composited surface appears in the grab is a device question, not a host one:
`tests/silica-stubs` stands in for `grabToImage`, and the real capture is on the
`docs/TESTING.md` checklist.
