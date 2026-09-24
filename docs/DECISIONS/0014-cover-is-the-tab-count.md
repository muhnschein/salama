# 0014 — The cover is the tab count over a field of page previews

## Context
The first cover showed the active tab: its favicon, its title over three lines, and
"*n* tabs" underneath in the small size. It read as a bookmark rather than as a
browser. The title is the one thing a cover cannot hold — it is foreign text of
unknown length in an unknown script, it changes on every navigation, and three
wrapped lines of it leave nothing for the cover to be. The favicon beside it is the
site's, so the cover took its colours from whatever page happened to be in front.

postivene and vuo had already settled the shape this project takes its measures
from: the app's name top left with one line under it saying what the number counts,
the number top right and large, and the rest of the cover given to texture made of
what the app actually holds.

## Decision
The cover says **how many tabs are open**, and shows them.

- The heading is postivene's exactly: "Salama" in the highlight colour at
  `fontSizeMedium`, `qsTr("Tabs")` under it at `fontSizeExtraSmall` in the secondary
  highlight, the two set `-Theme.paddingSmall` apart so they read as one heading, at
  `Theme.paddingLarge` from the top and left edges.
- The count is top right at `Theme.paddingMedium` from the top and
  `Theme.paddingLarge` from the right, `fontSizeHuge`, stepping down to
  `fontSizeExtraLarge` past 99 — where the text becomes "99+" — because a third
  glyph at the huge size runs into the app's name.
- Under it, `components/CoverTabField.qml`: the tabs in the model's own order, each
  the picture the tab grid already has for it (0008) on a faint rounded ground. The
  field begins `Theme.paddingLarge` below the heading and fades in over the next
  0.14 of the cover's height, which is the band vuo's texture leaves for the same
  heading.
- The field is **monochrome** — one `Desaturate` pass over the whole of it — and
  drawn at `Theme.opacityLow`. At full strength the desaturated pages read as a
  second screen inside the cover and take the eye off the number.
- The field is ordered **most recently in front first**, not by the grid's order. A
  cover is glanced at just after the app was put away, and what the glance should land
  on is where the reader has just been. `TabModel` stamps the tab in front with a
  counter — `Tab::lastActive`, `last_active` in the database from schema 3 — and
  `recentThumbnails` sorts a copy of the list by it, so neither the grid's order nor
  what is persisted is disturbed. The sort is stable, so tabs never yet in front keep
  the grid's order among themselves. On load the restored tab is stamped, which also
  gives a database written before schema 3 its first stamp.
- The grid is **shaped to the count**, not fixed: at most two across and three down,
  one column while there are two tabs or fewer, and the last row widened to take
  what its missing neighbours would have had. A fixed cell size cannot fill a cover
  at every count — one tab left a stamp in the corner of an empty cover and two left
  a row with a hole under it — and a field that only sometimes fills is not texture.

How much of this a cover shows is **`Settings.coverStyle`**, a choice of three, because
a cover is the one surface where taste is the whole argument and there is no reading of
it that is right for everyone:

- `CoverIconOnly` — the app's icon, semi-transparent and centred, and the action. For a
  reader who wants the switcher to stay a row of apps rather than a row of screens, and
  for whom a tab count is not news.
- `CoverLatestTab` — the heading and the number, over the one tab last read, drawn
  across the whole of the room below. The field shapes itself to what it is given
  (below), so this is the same component handed a list of one.
- `CoverEveryTab` — the heading, the number and the field. The default: what a reader
  who never opens Settings gets, and the reading this record argues for. Settings
  calls it "the tab count and the most recent tabs", because that is what the field
  shows: six cells at most, most recently read first, and the number above them is
  what says how many there are. "Every tab", which it said first, promised more than
  the field draws past the sixth.

The stored values are 0, 1 and 2 and are therefore part of the config file's format. A
value outside that range reads back as the default rather than as a cover that draws
nothing: the file is one a user can edit.

The one cover action is a **search**: `icon-cover-search`, opening a new tab with the
address field up and the whole url selected, so the first key typed replaces it. That is
what a browser is picked up for, and the cover is the one place where the choice of a
single action has to be right. It goes through `requestNewTab()` on the root window
rather than acting on `TabModel` itself: the address field belongs to the browsing page,
which a cover has no way to reach, and whatever page is on top — Settings, a dialog — is
popped first, or the new tab would arrive under a page that cannot type into it. The menu
is a sheet rather than a page, and puts itself away as the new tab comes to the front
(0021).

While the tab in front plays something, or is muted, its mute is a second action beside
the search, in a picture drawn for the cover (0025). The home screen draws whichever of
the two action lists is enabled, so there is one for each.

The app's name is held in a `brandName` property rather than written into the label:
a name is not a word to be translated, and `ci/qml-lint.sh` treats every bare string
in a `text:` binding as a defect, which is a rule worth keeping absolute.

## Consequences
The cover no longer names the page in front, and the device checklist asks for the
count and the field instead. A tab with no picture yet — one never displayed this
session — keeps its cell and shows the ground alone, so the field never contradicts
the number above it.
Past the sixth tab the field stops adding cells rather than shrinking them: it says
"a lot of tabs" as well as it is going to at cover size, and the number above it is
what says how many there actually are. The cells' pictures are drawn at whatever
scale their cell is, so a widened last row shows its page larger than the row above
it. That is a picture of a browser, not a table of pages, and the difference does not
read as an error at cover size.

The previews the field draws are as fresh as the cover needs them because `BrowserPage`
now also captures as the application leaves the screen (0008). Before that a preview was
only as new as the last load or the last visit to the grid, so a page that had been
scrolled, or stepped through without loading, was shown on the cover as it had been
before it was read.

Grey, not the site's colours, and no highlight on the active cell: a cover belongs
to the phone's ambience rather than to the pages inside the app, and a dozen
screenshots each in its own colours is noise at cover size. Which tab is in front is
not something the cover has room to say.

The icon the first style draws is `art/harbour-salama.png`, rendered from
`icons/harbour-salama.svg` by `icons/render.sh` and committed with the launcher icons.
It sits beside `qml/` rather than inside it — `ci/harbour-check.sh` holds that directory
to QML files alone, and that rule is worth more than the convenience of one shorter path
— and `CMakeLists.txt` installs `art/` next to `qml/` so the relative URL resolves the
same way in the source tree and on the device. It is a PNG rather than the SVG because
the SVG image format plugin is not something a Harbour application may count on.

`tests/silica-stubs/QtGraphicalEffects/` gains `Desaturate` and `LinearGradient`
beside `OpacityMask`, on the terms already written there: the module is not installed
on the host and the tests draw nothing, so the stubs exist for the import to resolve
and what the effects render stays a device question on the `docs/TESTING.md`
checklist.
