# 0032 — The start page is the home page

## Context
The home page was an address in Settings, Qwant's front page unless changed, and every new
tab and every first start loaded it. Firefox and Chrome open a page of their own instead:
Firefox's home offers shortcuts to the sites visited most, the pages read lately and the
bookmarks, each section switched on or off in its settings, or a blank page. A new tab in
sailfish-browser has no address at all and draws its favourites over the ambience, natively.

## Decision
There is no home page other than the start page. A tab with no address shows it: the grid's
plus, a first start, the tab left when the last one closes. Settings > Start page takes the
place of Settings > Home page (0028) and chooses what it shows — a blank page, or the
sections *Frequently visited*, *Bookmarks* and *Recently visited*, each on by default; the
sections keep their switches while the page is blank. The home page's address, and the ways
to take it from the page in front or a bookmark, are gone, and the address is taken out of
the settings file.

The page is Silica (`qml/components/StartPageView.qml`), not a document loaded in the engine,
as sailfish-browser's is: a tab on it has no `WebView` at all, so it costs nothing, shows at
once, and is drawn in the ambience's colours. `src/startpage/StartPage` reads its lists from
the database whenever the history or the bookmarks change: the sites visited most, one tile
per site (the host without "www.", as the bar shows it), the page of each read most; the
first eight bookmarks in their order; the five pages read last. A page of results from one
of the search engines on offer is in neither history list (`Settings::isSearchUrl`), or every
search would make its engine the site visited most. The tiles carry the page's icon, which
the history keeps for the address bar (schema 9), or the first letter of the host.

Typing an address on the start page, choosing one in the omnibar (0027), or tapping a tile
or a row, opens the page in that tab:
its view is made then. Back from the first page of it returns to the start page, as Firefox's
back does: the tab's Loader (`TabViewLoader.qml`) remembers where the page came from, since
the view does not, and `TabModel::showStartPage` takes the tab's address, title, icon and
preview away, and its view with them. The grid's picture of the start page is a grab of it,
taken where a page's would be.

A tab on the start page takes no place among the pages kept loaded (0016), is no visit, and
is not among the tabs closed lately (0018): there was nothing in it.

## Consequences
Anyone who had set a home page loses it; a bookmark is where it goes now, and shows on the
start page. There is no address for the start page: `about:home` typed in the bar is the
engine's. Forward from the start page back into the page just left is not offered — forward
has no control anyway (0009). A page given up past the limit of loaded pages comes back
without its history, and back from wherever it reloads at returns to the start page.

Tiles have no menu: Silica's context menu is made for list rows, and one under a tile in a
grid would open a quarter of the screen wide. A site leaves *Frequently visited* when it
leaves the history; the rows of *Recently visited* can remove their page themselves.

The per-tab Loader moved out of `BrowserPage.qml` into `TabViewLoader.qml` to keep the page
under the 600 lines 0010 allows it.
