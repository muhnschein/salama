# 0006 — Engine-facing strings live in C++

## Context
Clearing cookies and cache goes through `WebEngine.notifyObservers` with topics the
embedding understands (`clear-private-data`, payloads `cookies-and-site-data`, `cache`).
The scope forbids engine quirks in QML.

## Decision
`src/engine/EngineMessages` holds every engine-facing string, each with a comment naming
its upstream source, and is exposed as a QML singleton. QML passes the values to the
platform API without knowing them.

What the engine sends back on the topics it is asked to observe is read by the class
that acts on it: `PageActivity`, beside `EngineMessages` in `src/engine`, lists its two
topics and parses their payloads (`0020-pages-sleep-out-of-sight.md`), and
`DownloadModel` in `src/downloads` does the same for `embed:download`
(`0022-downloads-list.md`), so QML passes those on without knowing them either. How the
numbers in those payloads are read is `src/engine/EngineData`'s, for both. The script
that asks a page what it plays is `PageMedia`'s, beside the reading of its answer, which
decides what is paused next (`0025-media-controls.md`).

## Consequences
A change in the engine touches the class that reads what changed, and its unit test.
Linking `libsailfishwebengine` from C++ was rejected because it would need another host
stub for no gain.
