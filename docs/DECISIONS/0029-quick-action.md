# 0029 — The cover has one quick action, chosen in Settings

## Context
The cover's one action was a search that opened a new tab with its address up (0014): the
right single action for most readers, and the only one. piirit lets a reader choose its
cover's quick actions, two of them, among its pages, its search and one chat of the
reader's own, and keeps a chosen chat's action once the chat is gone. Here the cover's
second place is taken: it is the mute of the tab in front, which comes and goes by itself
while a tab plays (0026).

## Decision
**One action**, chosen on the cover's settings page (0028) under *Quick action*: none,
*Search*, *Bookmarks* (the list), *Open a bookmark* (one), *Downloads* or *History*. It is
`Settings.quickAction`, an unscoped enum stored as a number as the cover's style is, which
reads back out of range as the default, Search — what the cover offered before there was a
choice. A line over the choice says why there is one: the place beside it is kept for the
media control, which appears while the tab in front plays. Two small pictures of the cover
(`components/QuickActionPreview.qml`) show where the action goes with nothing playing —
alone, in the middle — and while a tab plays — left, the speaker right — at two thirds of a
real cover's width, drawn as the cover is set to show itself and with the files the cover
hands the home screen. With no action, a dot keeps its place.

**A row, not a ComboBox.** The choice is a `ListItem` drawn as Silica draws a ComboBox,
whose menu opens under it with the six. A ComboBox's value is the words of the item last
tapped, so *Open a bookmark* would read as chosen before a bookmark was, and after the
picker was backed out of; this row says what the action does now, "Bookmark: <its title>"
once there is one. A ComboBox would also move its choices onto a page of their own past
six, one kind more than there are; this menu opens where it is.

**The bookmark.** *Open a bookmark* pushes `pages/BookmarkPickerPage.qml`: the bookmarks
as their list draws them, without its menu, under a field that narrows them with the one
word matcher (0027) through `BookmarkModel.matching()`, asked again whenever the bookmarks
change (`BookmarkModel.revision`). A tap picks one, and only then is the action the
bookmark's; backing out has changed nothing. The setting keeps the bookmark's id with its
address and title beside it, written together (`Settings.setQuickActionBookmark()`). The
menu sheet's Bookmark, tapped twice, takes a bookmark away and adds it back under a new
id, so the bookmark is found by its id and then by its address, and the window points the
setting at what it finds without a word, on every change to the bookmarks — keeping the
address and title current, so that a bookmark renamed reads as it is called now, and one
whose address was edited is still found after coming back. Gone for good, the row reads
"Deleted bookmark" and the cover keeps the action, as piirit keeps a gone chat's; triggered
then, it opens the bookmarks, where another is a tap away.

A bookmark's action wears one of eight glyphs picked under the row — globe, heart, home,
work, news, music, shop, star — the star last and an outline, since the list of bookmarks
wears a filled star over lines, as the menu's Bookmarks is the filled star.

**The glyphs are drawn**, as the speakers are (0026). The home screen draws an action's
picture from its file as it is, so each glyph is in `icons/cover/`, in the speakers'
strokes, and `icons/render.sh` renders it into `art/cover/` at each size Silica's small
icon takes and in both inks. `Settings.coverIconPath()` names the file for a glyph, a size
and an ambience, and the cover's mute goes through it too. The theme's `icon-cover-search`
and its kin could not be matched to a drawn speaker in weight or size without the phone,
and the theme has no glyph for a bookmark of the reader's own.

**The cover** has three action lists, since the home screen draws the one enabled: the
quick action alone; the quick action and the mute, while the tab in front plays or is
muted; the mute alone when there is no quick action. With neither, none is enabled.

**The action is the window's** (`quickAction()`, `harbour-salama.qml`): the cover has
neither the page stack nor the browsing page in its scope. It pops to the browsing page
and puts away what lies over it — the menu's sheet, the address being edited, the grid
(`BrowserPage.uncover()`) — so that every action starts from the page; then it does the
action and activates the window. *Search* opens the address bar for a new tab (0027): empty, over
the bookmarks, no tab made until something is chosen, where it was a new tab loading the
home page with its address selected. *Bookmarks*, *Downloads* and *History* push their
pages, as the menu does. *Open a bookmark* brings the tab already showing it to the front,
in whichever group, the one most lately in front of several (`TabModel.tabIdForUrl()`), or
opens it in a new tab. The action comes before the window is activated, as the search
always did: the field has its focus as the window comes up, and the keyboard with it.

## Consequences
`Settings` stores the choice, the bookmark and its glyph, and `BookmarkModel` answers by id
and by address and counts its changes for bindings to read; the cover holds its lists and
their pictures, and none of what the action does. `BrowserPage.qml` grew by `uncover()` to
599 lines (0010).

`tst_settings` checks that `coverIconPath()` names an existing file for every glyph, at
every size and in both inks, and that the two inks differ. `quickActions`,
`quickActionLists`, `quickActionChoice` and `quickActionBookmarkFollows` in `tst_qmlload`
drive each kind from the cover, the lists with and without media, the row and its menu, the
picker, the glyphs, the pictures, and a bookmark renamed, re-added and deleted. How the
pictures and the glyphs read at their size, and each action from the home screen with the
application in the background, are device checks (`docs/TESTING.md`).
