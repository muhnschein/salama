# 0037 — The cover is where you were, over the halftone bolt

## Context
0031 made the cover the icon's bolt with a flash of sheet lightning, and nothing to read,
with 0014's tab count over the last tab as the one other choice in Settings. The bolt was
good to look at and said nothing a reader could use; the count said little more, and its
grey picture of the page still read as a second browser inside the cover. 0031 left
showing what plays for another day. What a cover is glanced at for, just after the app is
put away, is where the reader was, and whether something the browser is doing -- a
download, a video -- is still going.

Several covers were sketched at cover size, in a dark ambience and a light one, useful and
pretty and between the two, and cut down to one: the bolt as a field of dots, faint, under
what the browser has to say. A choice of covers in Settings goes with it: one cover that
says the right thing leaves nothing to choose.

## Decision
One cover, `cover/CoverPage.qml`, in four states, the first that applies:

- **Downloading** (`components/CoverDownloads.qml`), while `DownloadModel.runningCount` is
  above 0: Silica's `ProgressCircle` filled to `runningProgress`, the percentage large in
  it and "*n* files" under that. In **steps of five**, so the ring and the number change
  twenty times in a download at most, however often the engine reports.
- **Playing** (`components/CoverMedia.qml`), while the tab in front plays or is muted --
  when its mute is on the cover (0026). With a picture, the picture across the top, square,
  or 16:9 when it is clearly wider than tall, and under it the title, the artist, and
  "Playing · *site*" or, muted or paused, "Paused · *site*", the picture dimmed. Without
  one, the words over the halftone: "Playing", the site large, and the title.
- **Where you were** (`components/CoverPlace.qml`), while the tab in front has a page: the
  site in the highlight colour beside its icon (its first letter while it has none), the
  page's title large, as many lines as fit, and at the foot the tab's group when it is not
  the default one, and "*n* tabs".
- **The halftone alone**, whole, with no tab open or the one in front on the start page:
  there is nowhere to say the reader was.

**The halftone** (`components/CoverHalftone.qml`) is the launcher icon's bolt as dots on a
grid, largest inside the bolt and thinning towards its foot, with a halo just outside it
and a faint even field elsewhere, from the top edge down to the actions. `icons/render.sh`
computes it from the icon's outline and renders it once, white, into
`art/cover/halftone.png`; the cover tints it with the ambience's highlight colour
(`ColorOverlay`), whole on its own and at 0.12 under words -- `Theme.opacityFaint` left it
pulling the eye off the title. It is hidden under a picture of what plays.

**What plays is what the page says.** `PageMedia`'s script, which already asks each page
what it plays (0026), now answers JSON: the state, and while something plays or was paused
from here, the page's Media Session title, artist and widest artwork, or failing artwork
the poster of a video it plays. `TabModel` keeps it per tab only while the page plays
(`MediaMetadata`, `activeMediaTitle`, `activeMediaArtist`, `activeMediaArtwork`). Artwork
is taken only by an http or https address, which the cover's `Image` fetches itself, once
per address, decoded at the width it is shown at; not a `blob:`, which lives in the page,
nor a `data:`, which could be any size.

**Power.** Nothing on the cover animates; 0031's flash is gone. What it draws changes as
the tab in front, its page, the downloads' step or what plays changes, and is still in
between. The halftone is one picture drawn once.

The actions are as they were: the one quick action chosen in Settings (0029), and the mute
beside it while the tab in front plays (0026), in every state.

## Consequences
`CoverSettings.style` and its stored `coverStyle` key are gone; the key is left in the
file, unread. Settings > Cover is the quick action under one picture of the cover, the
halftone with the action on it (`components/QuickActionPreview.qml`), and the line under
Cover on the main settings page is the action alone, or "No quick action".
`components/CoverLightning.qml`, `CoverTabPicture.qml`, `icons/cover-bolt.svg` and
`art/cover/bolt.png` go, and `TabModel.recentThumbnails` with them, the cover being the
only reader; the groups' pictures keep `groupThumbnails`. `TabModel.currentGroupName`
names the group for the cover.

The cover names the page in front again, which 0014 moved away from: its title is foreign
text of unknown length, so it is cut to the room there is, and the site above it is what
reads at a glance. The site's icon is its own colours, small.

Whether the device's engine gives pages `navigator.mediaSession`, and how pages fill it, is
a device question; without it, a video's poster or the plain view is what shows. How each
state reads at cover size, in both ambiences, is on the `docs/TESTING.md` checklist.

`tst_pagemedia` runs the script's metadata over a fake Media Session and checks what is
kept of an answer; `tst_tabmodel` the metadata's life and the group's name; `tst_qmlload`
drives each state of the cover, the steps of five, and the settings page.

Supersedes 0031, which is kept for the history.
