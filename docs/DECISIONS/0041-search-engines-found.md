# 0041 — Search engines found while browsing

## Context
A page that has a search of its own says so: `<link rel="search"
type="application/opensearchdescription+xml" title="…" href="…">`. The engine's
`ContentLinkHandler.jsm` (embedlite-components) sends the application `Link:AddSearch`
with `{engine: {title, href}, url}`, once per document, for a link with a title and an
http, https or ftp address. sailfish-browser listens on every page that is not private
and adds the engine to its list at once (`apps/shared/WebView.qml`,
`apps/browser/settings/searchenginemodel.cpp`); its settings list the ones found as "Tap
to install". Salama had three engines, fixed, so the address bar could search only
where we had decided.

## Decision
**Every page is listened to, and nothing is added unasked.** `components/PageSearchLink.qml`
registers the message on each view as `PageNotificationLink` registers its own, and hands
what it says -- read by `EngineMessages::searchOffered`, the name and the shape of the
message being the engine's (0006) -- to `SearchEngines::offerEngine`. An offer is kept
as `{title, href, host}` unless an engine of that title is on offer or already added, or
the offer is already kept. There is no private mode (0019) to keep it from.

**Taking one up is a tap on it in Settings > Search.** `components/SearchEngineInstaller.qml`
fetches the description with QML's own `XMLHttpRequest`, which is Qt's network access and
adds no module Harbour does not list, and hands the text to `SearchEngines::addFoundEngine`.
`OpenSearch::parse` (`src/search`, `QXmlStreamReader`, QtCore) reads it: the `ShortName`,
and the first `<Url>` that is `text/html`, method GET, rel `results` -- its `<Param>`s
added to the query, every parameter but `{searchTerms}` left empty. A template is kept
only on http or https, with `{searchTerms}` and not in the host; text that is not
well-formed, or more than 64 KiB, is no description. The status line is not asked: a
page that is not a description fails to read. The engine is added, chosen and no longer
an offer, and a Silica `Notice` says "%1 search added"; otherwise "Could not add %1" and
the offer stays, to be tried again or forgotten.

**The engines are the built-in three and the added ones, and are `SearchEngines`' to keep.**
It is a section of the settings file of its own (`src/search/SearchEngines.h`), made
before `SearchSettings`, which borrows it and keeps the choice among them. `engineNames`,
`engineKeys` and `engineHosts` notify; `engineIndex` moves with the list. When an offer is
taken up `SearchEngines` says `engineAdded(key)`, before it says the list changed, and
`SearchSettings` chooses that engine. The list and the choice are two classes because
together they were more methods than one class should have. Added engines are kept in the
settings file as JSON (below), with the host of the page that offered them, shown as
"Added from %1". Removing one, or all of them with the page's pull-down menu after a
remorse, has `SearchSettings` put the first built-in engine in use if it was theirs, and
write that, so the file never names an engine that is not in it. Built-in templates now
read `{searchTerms}` like the rest.

**`isSearchUrl` is no longer static.** Which pages are results depends on the engines,
so `StartPage` takes the `SearchEngines` it asks, Core has it refresh when the list
changes, and each engine's host, path and query parameter -- or, for an engine with the
words in its path, what stands before and after them -- is read once per change.
An engine whose words are the whole path would take in its whole site and is not read.

## Consequences
New keys in the settings file: `searchEnginesAdded` and `searchEnginesFound`, JSON arrays of
records, read back with each record checked, since the file is editable. The offers are
kept without limit but one per title; a site that offers many is a list to forget.
Engines that want POST, an XML results format, or another way to search are not added.
`tst_opensearch` and `tst_searchengines` bite the reading, keeping, adding, removing and
the fallback; `tst_qmlload` serves descriptions from the loopback and taps through the
page. Whether Silica opens the context menu from a press and hold on a `TextSwitch` the
row passes on, as in the stubs, is a device check (`docs/TESTING.md`).
