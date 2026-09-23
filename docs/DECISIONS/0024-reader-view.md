# 0024 — The reader view is Firefox's, run in the page

## Context
Firefox and Mobile Safari both offer a page's article alone — its title, its byline and
its text, set in type chosen for reading, without the site around it. What was asked for
is the same here, reached from the menu's *This page* row (0021), borrowing from Firefox
where the licences allow. They do: Firefox's reader view is Mozilla's Readability
(`mozilla/readability`, Apache-2.0), which decides whether a page is an article and finds
it, and `toolkit/components/reader` with `toolkit/themes/shared/aboutReader.css`
(MPL-2.0, this project's licence), which show it on a page of its own, `about:reader`.

None of Firefox's machinery around them is there to use. `about:reader` is a privileged
page driven by actors desktop Firefox registers; embedlite registers none, and Harbour
allows nothing private to reach them (`docs/HARBOUR.md`). Readability works on a DOM,
which only the engine has — Qt 5.6 has no HTML DOM to port it to, and SCOPE.md §3 rules
out any application language but QML and C++.

## Decision
**Readability runs in the page, as the engine's JavaScript.** It is the web platform's
language in the web platform's engine, as the favicon's and the theme colour's scripts
already are (0005, 0013), not a third language for the application. The two files are
Mozilla's, byte for byte, under `third_party/readability/` with their licence, notice and
the commit they came from (`UPSTREAM`); they are compiled in as Qt resources and handed to
`runJavaScript` whole, wrapped by `src/reader/Reader.cpp` in what Firefox's own callers
add: `Readerable.js`'s check — HTML documents only, a node counting if it takes up room,
no site's front page, and Firefox's list of sites that read as articles and are not —
and `ReaderMode.sys.mjs`'s options, the class names its style sheet matches. Readability
parses a copy of the document, as Firefox's worker parses a copy, so the page is not
touched. Where Firefox then puts the article through its sanitizer on the way into
`about:reader`, the wrapper takes out the same things before handing it back: scripts,
frames, form controls, handlers and script urls. Readability's code stays outside `src/`
because it is not this project's to edit: the TODOs in it are Mozilla's.

After every load of a page that is not a reader view, the view asks whether it reads as an
article; the menu's *Reader view* entry is dimmed unless it does, and lit while the reader
view is shown.

**The reader view is a page of its own, as `about:reader` is.** `Reader::page()` sets the
article in `about:reader`'s markup — the site, the title, the byline, Firefox's reading
time estimate, a rule, the article — styled by `src/reader/reader.css`, which is
`aboutReader.css` less Firefox's own chrome (its toolbar and popups, the themes this
application does not offer, print rules), with the design tokens it reads from `chrome://`
written in, two rules Firefox applies from its page script — centring block images,
scrolling wide tables — written as plain CSS, since this page runs no script of its own,
and lists kept to the text's width: Firefox's pull their end out 10px a level, which its
desktop margin hides and a phone's 20px does not. The view loads it through the
platform's `loadHtml`, which makes a `data:` url of it (qtmozembed
`QuickMozView::loadText`). So, as in Firefox, it is an entry in the view's history: back
leaves it, the menu entry leaves it by going back, and a link in it opens in the tab. A
Content Security Policy in its head lets nothing of the article run or load but pictures
and media; evaluated scripts stay allowed, because that is what the engine's
`runJavaScript` is.

**The tab never sees the `data:` url.** The document's first element names the page it was
made from, and `Reader::sourceUrl()` reads it back out of the address the engine reports.
The tab — and so the bar, the history, bookmarking and sharing — keeps the article's own
address; the favicon is the article site's. Reading it from the address rather than
remembering it means a reader view reached by back or forward, after a link followed from
it, is known for what it is too. The document's `<title>` is the page's own, so the tab
keeps its name, and its `theme-color` is the reader theme's background, which the strip
beside the cutout takes (0013). The engine calls a `data:` document insecure, which says
nothing about the article's connection, so the bar's warning of a broken one (0011) is not
drawn over a reader view.

**Its look is Settings', as Firefox's is its preferences'.** Colours — the ambience's,
light or dark as it is, or Firefox's light, sepia or dark whatever the ambience —
typeface, sans-serif or serif, and Firefox's nine text sizes (`10 + 2 × step` pixels, five
the default), in a *Reader view* section of Settings. A reader view on the screen follows a
change at once: the view is handed a script that sets its classes and font size, as
Firefox's page sets its own, rather than loaded again and scrolled back to the top.

The entry's icon is `icon-m-file-formatted`, the one Jolla's Documents gives a text
document (sailfish-office `plugin/TextDocumentPage.qml`), for the reason 0021 gives for
the names it uses. It is the fifth in its row, so *This page* runs to a second line.

## Consequences
Readability runs in the page's own JavaScript world, where Firefox runs it in a worker
with a DOM of its own. A page that has replaced the built-ins Readability uses can break
it; the menu entry then does nothing and the page stops being offered. A long page is
parsed on the page's thread, for as long as that takes, when the entry is tapped.

The reader view is not restored: a tab restored at startup, or reloaded past the limit of
loaded pages (0016), comes back as the article's page. Firefox's controls inside the
reader view — its toolbar, content width, line height, text-to-speech — are not here; the
page's own pinch zoom still works. Only `http` and `https` pages are offered one.

The article's pictures are fetched by a document with no origin of its own, so without a
referrer; a site that refuses pictures to requests without one shows none.

What the reader view needs of the browsing page is two lines on each view, the rest being
`components/ReaderMode.qml`. With them, and tracking protection (0023), the page is at the
600-line ceiling 0010 sets; the next thing it gains wants the split 0010 names first.

Readability is refreshed by copying the four files from a newer commit and updating
`UPSTREAM`; `tests/tst_reader.cpp` checks that the scripts carry the files as they are.
The host's QJSEngine cannot parse Readability's newer syntax, so the tests compile only
what this application wraps around it; that it runs is the device's to show
(`docs/TESTING.md`).
