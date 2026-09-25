# 0030 — History has a settings page of its own

## Context
Clearing browsing data was an entry on the privacy page, under tracking protection
(0028), and on the phone it was lost there: what someone looks for to forget where they
have been is the history, not privacy in general. Firefox keeps a *History* section in its
settings — "Remember history", or custom settings: *Remember browsing and download
history*, *Clear history when Firefox closes*, and a *Clear History…* button to a dialog
that asks how far back (*Last hour*, *Last two hours*, *Last four hours*, *Today*,
*Everything*) and which kinds (`browser/components/preferences`, `sanitize.ftl`). Firefox
for Android has *Delete browsing data* and *Delete browsing data on quit* as entries of
their own.

## Decision
**A History page** (`pages/HistorySettingsPage.qml`), reached from the main settings'
*Privacy* heading, after the privacy page, by `icon-m-history`, the menu's own for the
history. On it, Firefox's three:

- **Remember browsing history** (`Settings.rememberHistory`, on until switched off). Off,
  a page visited is not written to the history (`Core` asks before it passes a visit on),
  and nothing is learnt for the address bar (0027); what was kept stays until it is
  cleared, as in Firefox. The downloads list is not part of it: it lists files the phone
  now holds, which the switch would not unmake.
- **Clear history when closed** (`Settings.clearHistoryOnClose`, off until switched on):
  the history, with what the address bar learnt, the list of downloads and the recently
  closed tabs go as the application quits (`Core::clearOnClose`, from `main()` on
  `aboutToQuit`) — and as it starts, for an application stopped before it could close,
  which on a phone is how it often ends; Firefox clears at its next start for the same
  reason. The open tabs and the bookmarks stay: they are not history.
- **Clear browsing data**, the dialog 0028 made, moved here with the remorse that runs
  it and the `Sailfish.WebEngine` import the engine's clearing needs. The dialog now asks
  **how far back**, Firefox's ranges, everything to begin with, as before. The history —
  now *Browsing and download history*, Firefox's words — takes the pages last visited in
  the range, what was learnt in it, and the downloads started in it but for any still
  coming; the recently closed tabs go with the whole history. The rest — open tabs,
  cookies and site data, the cache — has no time to be cleared by and is cleared whole,
  which a line under the range says once one is chosen. A page visited before the range
  and again in it goes whole: a row keeps only its last visit.

The privacy page keeps tracking protection alone.

## Consequences
`HistoryModel` gained `clearSince()` and `rangeStart()`, `DownloadModel` `clearSince()`;
`tst_historymodel`, `tst_downloadmodel`, `tst_settings` and `tst_core` test them and the
switches, and `historySettingsPage` and `clearDataDialog` in `tst_qmlload` drive the page.
Whether a quit from the home screen clears before the process goes, and the next start
after one killed does, are device checks (`docs/TESTING.md`).
