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
**The main page** (`pages/SettingsPage.qml`) keeps what takes a line, and leads to the rest.
Under *General*, Search first — what the bar searches with and suggests from is what a
browser is used for most, and Firefox puts it first too — then the home page, desktop
sites, the cutout and the pages kept loaded, as they were. Under *Appearance*,
sailfish-browser's heading for how the browser and its pages look: the reader view and the
cover. Under *Privacy*, privacy. Each way in is a `components/SettingsEntry.qml`: a theme
icon, the subject's name, and under it a line saying how the subject is set now — "Qwant",
"Ambience · Sans serif · 100 %", "The tab count and the most recent tabs · Search",
"Tracking protection: Standard" — in the words its page offers the choices in, and bound to
the settings it names, so a change made on the subject's page is there when it is popped.
The main page is where the settings are read, not only where they are reached.

**The subjects' pages.** *Search*: the engine, and under *Address bar suggestions* a switch
for each source the bar suggests from — open tabs, bookmarks, history, downloads (0027).
*Reader view*: its colours, typeface and text size (0024). *Cover*: what the cover shows
(0014) and its quick action (0029). *Privacy*: tracking protection (0023), and the way to
clear browsing data. The controls moved as they were, and each still writes its setting as
it changes; no page has a Save.

The icons are those Jolla's applications give the same subjects, cited where they are
used: `icon-m-search` and `icon-m-delete` from sailfish-browser's settings,
`icon-m-device-lock` from its certificate view, the menu's `icon-m-file-formatted` for the
reader view (0024). For the cover, `icon-m-tabs`, which sailfish-browser's toolbar writes
its tab count into — what the cover is here — and not `icon-m-display`, which
sailfish-browser gives its notch guard, the screen cutout here.

**Clearing is a dialog** (`pages/ClearDataDialog.qml`), as sailfish-browser asks it: a
switch for each kind, in the order Firefox for Android lists them — open tabs, history,
cookies and site data, cache — and Clear dimmed while none is on. What forgets where one
has been is on to begin with; the open tabs are not, since closing them takes away what is
being read. The dialog only asks. The privacy page, on the screen again as the dialog goes,
runs one remorse for whatever was chosen, and then clears each kind as its button did, in
the dialog's order: one decision, undone at once if it was the wrong one, where four
buttons were four remorses. The choices are copied out as the dialog is accepted, since the
page stack does away with it as it is popped.

sailfish-browser also asks how far back the history goes, and Firefox also lists site
permissions and downloads. Not here: the history is cleared whole, as it was; permissions
are the engine's, and showing them is Phase 2's (SCOPE.md §6); the downloads list has its
own Clear list (0022).

**The engine import moves with the clearing.** The main page imported `Sailfish.WebEngine`
to notify the engine to clear its cookies, site data and cache; the privacy page imports it
instead, and `tests/tst_qmlstatic.cpp` allows it there and in `BrowserPage.qml` alone.

## Consequences
A subject is a tap further away: the price of a main page that fits on a phone's screen,
summaries and all. A subject that grows past a line gets a page and an entry, not a
section.

The load tests reach each page by tapping its entry (`settingsPage`, `searchSettingsPage`,
`readerSettingsPage`, `privacySettingsPage`, `coverSettingsPage`) and check each summary
follows its setting; `clearDataDialog` drives the switches, the dimmed Clear, the remorse —
the stub `Remorse` now records which page it was shown on — and what is cleared. Whether
the icons are in the device's theme, whether a long summary fades rather than wraps, and
whether the remorse shows on the privacy page as the dialog leaves are device checks
(`docs/TESTING.md`).
