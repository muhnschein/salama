# 0036 — Settings say how they are set, and choose in place

## Context
Settings had become a menu of names (0028): the main page said nothing of how anything
was set, and every page under it made its choices in combo boxes — the start page's, the
search engine, tracking protection, the reader view's colours and typeface, the cover's
style — so each choice was a tap to see what else there was and another to make it, and
the cover's quick action a row whose menu hid the six actions. Lines under switches said
again what their names said. The history page cleared browsing data without saying what
there was to clear, and the dialog that clears it asked which kinds without saying how
much of each. The notifications page wrote each site's status on a line under it.

Silica itself shows a setting with its value: a `ComboBox` writes it beside its label in
the highlight colour, a `DetailItem` beside its own. A redesign of every settings page,
drawn as Silica pages over the device's ambience, was taken as the way forward.

## Decision
**The main page says how each subject is set** (`components/SettingsEntry.qml`): under
its name, in `Theme.secondaryHighlightColor` and the extra small size, as Silica writes a
value under a label — "Your sites", "Qwant", "Ambience · Sans serif · 100 %", "Lightning ·
Yle Uutiset", "Standard", "2 sites allowed · 1 blocked", "Remembered". The tutorial's way
in has none and is its name alone, centred. The words are the choices' own, from one
place (`components/SettingNames.qml`), so the page a choice is made on and the line that
says it agree; the notifications line asks the engine for the sites as the page opens.
This reverses 0028's revision, which took these lines out when they were long sentences:
they are now the choice's name, one line each, and the page still fits the screen. The
headings are *Browsing*, for what browsing starts from and goes by, *Appearance*,
*Privacy* and *Help*; *Privacy*'s one page is named for what is on it, *Tracking
protection* (`pages/TrackingSettingsPage.qml`), so the heading is not said twice. The
website colours join the cutout's switch under Appearance (0035); the switch's line went,
its name saying what it does.

**A choice among a few is laid out where it is made.** Silica has no radio button: each
choice is a `TextSwitch` that does not check itself (`automaticCheck` off), checked while
it is the one set, so its light is the choice's — the start page's *Your sites* and
*Blank page*, the search engines under a heading of their own, the three tracking levels
each with the line saying what it does. What is better seen than named is shown: the
reader view's colours are five squares painted as the reader view will be
(`components/ReaderSwatch.qml`) and its typefaces two tiles each written in its own face
(0024); the cover's style is its two pictures side by side, the one set ringed, each with
the quick action where the home screen draws it (`components/QuickActionPreview.qml`,
0031); the quick action is six rows, each with the glyph it wears on the cover and the
one set lit, the bookmark's naming its bookmark under it, and under the rows the glyphs a
bookmark's action can wear (0029). The second picture of the cover, drawn while a tab
plays, gave way to the line that says the tab's mute sits beside the action.

**What a page does is said, and only that.** A line under a switch stays where it says
more than the name — what goes with the history as the browser closes, what each tracking
level does — and goes where it said the name again. The start page shows, under the
heading *Preview*, the screen as a new tab will show it, at half its size and following
the switches (`components/StartPagePreview.qml`): the start page's own tiles and rows,
with the reader's own sites, laid out at the screen's size and made smaller, and the
navigation bar along the foot as a new tab has it. A section with nothing in it yet is
drawn as where its tiles and rows go, so a switch always shows what it does; a blank
page is the bar alone, as it is. The history page counts what is kept on the phone, as Silica lays out details
(`DetailItem`): the pages the history holds (`HistoryModel.pageCount`, the whole table
rather than the page of it the list shows), the downloads, the recently closed tabs and
the open tabs with their groups; *Clear browsing data* is a Silica `Button` under them,
since it does something, after asking what, rather than going somewhere (0030). The
dialog says under each kind how much goes: the open tabs in every group, the pages,
downloads and closed tabs of the range chosen (`HistoryModel.countSince`,
`DownloadModel.countSince`, the closed tabs only with everything), and that clearing
cookies signs you out of most sites. The notifications page lists the sites under
*Allowed* and *Blocked*, the model keeping the allowed first and moving a row under the
other heading as it is switched, rather than taking it out and putting it back
(`NotificationPermissions`, 0033); its switch reads *Sites can ask*, the way round a
switch that is on reads, and *Remove* is *Forget this site*, with a line under the list
saying what that does.

## Consequences
Every choice is on the screen with the others, so a page is read at a glance and changed
with one tap; the price is length, and the cover's page scrolls on the phone. The words
of each choice live in `SettingNames.qml`, whose context translators see once. The load
tests reach each page as before and drive the choices by `objectName`
(`searchEngineChoice`, `trackingProtectionChoice`, `readerColorsChoice`,
`readerTypefaceChoice`, `coverStyleChoice-*`, `quickAction-*`, `startPagePreview*`), checking that each line
under a way in follows its setting wherever it is written from; `tst_historymodel`,
`tst_downloadmodel` and `tst_webnotifications` check the counts and the order. The stubs
gain `DetailItem` and `Theme.iconSizeExtraSmall`. How the squares, tiles and pictures look
in a light and a dark ambience is a device check (`docs/TESTING.md`).
