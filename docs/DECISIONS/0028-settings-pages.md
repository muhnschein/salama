# 0028 — Settings are a main page with a page per subject

## Context
Settings had grown into one long page: the home page, the search engine, desktop sites,
the cutout and the pages kept loaded, then under headings of their own the reader view's
three controls, tracking protection, what the cover shows, and four buttons that cleared
the history, the cookies and site data, the cache and the open tabs, each under a remorse
of its own. The address bar's sources (0027) and the cover's quick action (0029) would each
have made it longer, and it was already a page to scroll through for a thing rather than
one to read.

Firefox for Android heads its settings in groups — General, Privacy and security — and
gives each subject that takes more than a line a page of its own: Search, Enhanced
Tracking Protection, Delete browsing data. Jolla's own browser keeps its settings on one
page under the headings Privacy, Downloads and Appearance, and asks what to clear in a
dialog of switches (`apps/browser/qml/pages/PrivacySettingsPage.qml`).

## Decision
**The main page** (`pages/SettingsPage.qml`) leads to the subjects. Under *General*, the
home page first, as sailfish-browser puts its own first, then Search — what the bar
searches with and suggests from, which Firefox for Android puts first of its own. Under
*Appearance*, sailfish-browser's heading for how the browser and its pages look: the
reader view, the cover, and last the one setting that takes a line, the screen cutout's
switch (0013). Under *Privacy*, privacy and the history (0030). Each way in is a
`components/SettingsEntry.qml`: a theme icon and the subject's name, nothing under it. The
cutout's switch keeps the line that says what it does; nothing else on the page has one.

*Revised.* Each way in first had a line under its name saying how the subject was set —
"Qwant", "Ambience · Sans serif · 100 %", "The tab count and the most recent tabs ·
Search" — and the main page kept the home page's field, *Request desktop sites* and
*Pages kept loaded*. On the phone the lines made a page of lists to read rather than a
menu to pick from, so they went. The home page got a page of its own; the other two went
from Settings altogether: a page's desktop version is the menu's switch (0021), which is
where it is wanted, and how many pages stay loaded is the platform's five (0016).

**The subjects' pages.** *Home page*: its address typed, the page in front or a bookmark
taken as it is, as Firefox's Home settings offer them, or the default again
(`pages/HomePageSettingsPage.qml`, the bookmark picked as the cover's is, 0029). *Search*: the engine, and under *Address bar suggestions* a switch
for each source the bar suggests from — open tabs, bookmarks, history, downloads (0027).
*Reader view*: its colours, typeface and text size (0024). *Cover*: what the cover shows
(0014) and its quick action (0029). *Privacy*: tracking protection (0023). *History*: whether
it is kept and cleared on closing, and the way to clear browsing data (0030). *Reader
view* shows a few lines of an article under its controls, set as they set it (0024). The
controls moved as they were, and each still writes its setting as
it changes; no page has a Save.

The icons are those Jolla's applications give the same subjects, cited where they are
used: `icon-m-home` from sailfish-browser's home page setting, `icon-m-search` and `icon-m-delete` from sailfish-browser's settings,
`icon-m-device-lock` from its certificate view, the menu's `icon-m-file-formatted` for the
reader view (0024) and `icon-m-history` for the history. For the cover, `icon-m-tabs`, which sailfish-browser's toolbar writes
its tab count into — what the cover is here — and not `icon-m-display`, which
sailfish-browser gives its notch guard, the screen cutout here.

**Clearing is a dialog** (`pages/ClearDataDialog.qml`), as sailfish-browser asks it: a
switch for each kind, in the order Firefox for Android lists them — open tabs, history,
cookies and site data, cache — and Clear dimmed while none is on. What forgets where one
has been is on to begin with; the open tabs are not, since closing them takes away what is
being read. The dialog only asks. The history page (0030; the privacy page, before it), on the
screen again as the dialog goes,
runs one remorse for whatever was chosen, and then clears each kind as its button did, in
the dialog's order: one decision, undone at once if it was the wrong one, where four
buttons were four remorses. The choices are copied out as the dialog is accepted, since the
page stack does away with it as it is popped.

sailfish-browser also asks how far back the history goes, and Firefox also lists site
permissions and downloads. How far back is asked now, with the downloads going with the
history (0030); permissions are the engine's, and showing them is Phase 2's (SCOPE.md §6).

**The engine import moves with the clearing.** The main page imported `Sailfish.WebEngine`
to notify the engine to clear its cookies, site data and cache; the page that clears
imports it instead — the history page, since 0030 — and `tests/tst_qmlstatic.cpp` allows it
there and in `BrowserPage.qml` alone.

## Consequences
A subject is a tap further away, and how it is set is read there: the price of a main
page that fits on a phone's screen and reads at a glance. A subject that grows past a
line gets a page and an entry, not a section.

The load tests reach each page by tapping its entry (`settingsPage`, `searchSettingsPage`,
`homePageSettingsPage`, `readerSettingsPage`, `privacySettingsPage`,
`historySettingsPage`, `coverSettingsPage`); `clearDataDialog` drives the switches, the dimmed Clear, the remorse —
the stub `Remorse` now records which page it was shown on — and what is cleared. Whether
the icons are in the device's theme and whether the remorse shows on the history page as the dialog leaves are device checks
(`docs/TESTING.md`).
