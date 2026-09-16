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

- The heading is postivene's exactly: "Tuuli" in the highlight colour at
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
- The grid is **shaped to the count**, not fixed: at most two across and three down,
  one column while there are two tabs or fewer, and the last row widened to take
  what its missing neighbours would have had. A fixed cell size cannot fill a cover
  at every count — one tab left a stamp in the corner of an empty cover and two left
  a row with a hole under it — and a field that only sometimes fills is not texture.

The app's name is held in a `brandName` property rather than written into the label:
a name is not a word to be translated, and `ci/qml-lint.sh` treats every bare string
in a `text:` binding as a defect, which is a rule worth keeping absolute.

## Consequences
The cover no longer names the page in front, and the device checklist asks for the
count and the field instead. A tab with no picture yet — one never displayed this
session, and every private tab, whose pages are never written to disk — keeps its
cell and shows the ground alone, so the field never contradicts the number above it.
Past the sixth tab the field stops adding cells rather than shrinking them: it says
"a lot of tabs" as well as it is going to at cover size, and the number above it is
what says how many there actually are. The cells' pictures are drawn at whatever
scale their cell is, so a widened last row shows its page larger than the row above
it. That is a picture of a browser, not a table of pages, and the difference does not
read as an error at cover size.

Grey, not the site's colours, and no highlight on the active cell: a cover belongs
to the phone's ambience rather than to the pages inside the app, and a dozen
screenshots each in its own colours is noise at cover size. Which tab is in front is
not something the cover has room to say.

`tests/silica-stubs/QtGraphicalEffects/` gains `Desaturate` and `LinearGradient`
beside `OpacityMask`, on the terms already written there: the module is not installed
on the host and the tests draw nothing, so the stubs exist for the import to resolve
and what the effects render stays a device question on the `docs/TESTING.md`
checklist.
