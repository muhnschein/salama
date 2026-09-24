# 0029 — The cover has one quick action, chosen in Settings

## Context
The cover's one action opened a new tab. Piirit lets a reader choose its cover's
actions; here the cover's second place belongs to the mute of a tab that plays
(0026).

## Decision
One action, chosen on the cover's settings page: none, search, the bookmarks, one
bookmark, the downloads or the history. A chosen bookmark is kept by its id with its
address and title beside it, so it can be found again after being removed and added
back, and it wears one of a set of glyphs drawn for the cover.

## Consequences
`Settings` stores the choice and names the glyphs' files (`coverIconPath()`), and
`BookmarkModel` and `TabModel` answer by id and by address for the action to resolve.
