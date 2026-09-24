# 0028 — Settings are a main page with a page per subject

## Context
Settings had grown into one long page holding the search engine, the reader view's
look, tracking protection, data clearing and the cover, one after another.

## Decision
The main page keeps the general settings and leads to a page each for search, the
reader view, privacy and the cover, as Firefox for Android and Jolla's own browser
arrange theirs; clearing browsing data becomes a dialog on the privacy page.

## Consequences
`Sailfish.WebEngine` moves with the data clearing to the privacy page, and
`tests/tst_qmlstatic.cpp` allows it there.
