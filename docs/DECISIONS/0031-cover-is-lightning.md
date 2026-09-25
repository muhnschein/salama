# 0031 — The cover is lightning

## Context
0014 made the cover the tab count over a field of the open tabs' previews, with two
other styles in Settings: the icon alone, and the count over the tab last read. None of
the three was right in use. For most readers the count is not news, the field of grey
screenshots read as a second, smaller browser inside the cover, and the icon alone was
the launcher's icon again, shrunk and dimmed. What a cover has to *do* its actions
already do: the quick action chosen in Settings (0029), and the mute while the tab in
front plays (0026). What was
missing was a cover that is good to look at and asks nothing of the reader.

Three were sketched at cover size, in a dark ambience and a light one: a bolt generated
anew from the front tab's host; an afterglow, the front page blurred past reading into
a wash of its colours; and the icon's own bolt with a flash of sheet lightning. The
generated bolt has to look good for every host, and some seeds will not. The afterglow
brings back the site's colours 0014 kept off the cover, and a dark page in a light
ambience turns it muddy. The icon's bolt is the application's name — *salama* is Finnish
for lightning — is drawn once, and is the picture the reader already knows it by.

## Decision
By default the cover is **the bolt and nothing to read**: `Settings.CoverLightning`,
drawn by `components/CoverLightning.qml`.

- The launcher icon's bolt, alone: `icons/cover-bolt.svg`, rendered once by
  `icons/render.sh` into `art/cover/bolt.png`, white, fading from 95% at the tip to 20%
  at the foot. The cover tints it with the ambience's highlight colour (`ColorOverlay`)
  at `Theme.opacityHigh`, so it takes on whatever ambience the phone has.
- Large and cropped. Its box is given in parts of the cover: 0.235 of the height above
  the top edge, 0.376 of the width in, 0.646 wide and as tall as the cover. The tip runs
  off the top right and the foot stops three quarters of the way down, short of the
  actions, on a large cover and a small one alike. The picture is stretched to that box,
  and rendered at its aspect so the stretch does not show.
- No name and no number. The bolt is the icon, and the icon is how the home screen tells
  one application from another.
- Each time the cover comes into view — its `status` turning `Cover.Active` — **one flash
  of sheet lightning**: a quarter of a second's pause, a rise to 85%, a dip to 30%, a
  second and brighter rise, and a 1.2 s fade, about two seconds in all, and then
  stillness. The light is a soft radial glow behind the bolt's upper arm. In a dark
  ambience it is in the highlight colour, and the bolt whitens with it; in a light one
  it is white, because a glow in a darker colour reads as a smudge there. As the cover
  goes out of view the flash is put out at once, and nothing runs while it is out of
  view. Two peaks half a second apart is far under what counts as flicker.

The last tab stays, **for readers who want the cover to say something**:
`Settings.CoverLatestTab`, 0014's heading and number over the tab last in front, drawn by
`components/CoverTabPicture.qml`, the field cut down to its one-picture case. The
icon-only and every-tab styles are gone, and the six-cell field with them.

The stored values are still the config file's format. 0 was the icon alone and is now
the lightning; 1 was and is the last tab; the every-tab cover's 2 is out of range, and
reads back as the lightning as any stray value does. A reader who chose the last tab
keeps it, and everyone else gets the new default.

## Consequences
Unless it is set to the last tab, the cover says nothing a reader can act on, and its
actions are the whole of its use. Showing what is playing on the cover is left for
another day, and has room: the middle and foot of the lightning cover are empty.

The glow and the tint are `QtGraphicalEffects` (`RadialGradient`, `ColorOverlay`), which
`tests/silica-stubs/` now stubs beside the others, on the terms written there. The stubs'
`CoverBackground` gains a `status` and `Cover` its values, so the tests can bring the
cover into view and watch the flash start, finish and be put out. What the flash looks
like, and that the home screen sets the status it plays on, are device questions on the
`docs/TESTING.md` checklist.

The two small covers on the cover's settings page (0029) draw the lightning with the
cover's own component, never brought into view, so still; the icon-only and every-tab
pictures they had go with the styles. `art/harbour-salama.png`, which only the icon-only
cover and its picture there drew, is no longer rendered or installed, and the omnibar's
test that borrowed it as a site's icon takes the 86-pixel launcher icon instead.

Supersedes 0014, which is kept for the history and for the heading's measures, which the
last-tab cover still uses.
