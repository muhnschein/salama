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
that acts on it, beside `EngineMessages` in `src/engine`: `PageActivity` lists its two
topics and parses their payloads (`0020-pages-sleep-out-of-sight.md`), so QML passes
those on without knowing them either.

## Consequences
A change in the engine touches one file and one unit test. Linking `libsailfishwebengine`
from C++ was rejected because it would need another host stub for no gain.
