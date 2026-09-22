# Changelog

All notable, user-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow semantic versioning.

## [Unreleased]

### Fixed
- Pages play sound. The application now holds Sailjail's `Audio` permission, without which the platform keeps it out of the sound system altogether.
- The grid can be pulled back to the page from anywhere on it again, a preview or the row of groups included, and a grid longer than the screen scrolls from a drag begun on a preview. A preview being held is let go when the finger moves up or down, as a held list item is.

### Changed
- A new application icon: a lightning bolt on a dark disc in a gold frame.
- A preview in the grid is picked up after a second of holding rather than a second and a half.
- Private tabs are gone, and the private group with them: the platform offers a Harbour application no way to put a device-lock or fingerprint gate on them, and an unlocked "Private" group would promise what it cannot keep. A database that still has private tabs loses them on first start.
- The application is now called salama (it was tuuli): package `harbour-salama`, Sailjail application name `salama`. The data directory changes with the name, so tabs, bookmarks, history and settings from a tuuli build do not carry over.

### Added
- Multi-tab browsing with a grid of page previews; tabs, the active tab and the previews survive restarts.
- Dragging the navigation bar upwards pulls the tab grid up from under the page, and dragging the grid down past its top, from anywhere on it, puts the page back; tapping a preview does the same. A handle on the bar's edge and a line across the top of the grid say where to take hold. The menu reaches the grid without the gesture.
- Address shown in the navigation bar as the host alone, tapped to edit the whole url in place, opening addresses or searching with a configurable engine. Scrolling down slims the bar to the address alone and lets the page run behind it; scrolling up brings it back whole.
- A red warning on the address when the engine reports a broken TLS connection, and a navigation bar the page ends above rather than running behind, so the foot of a page is always reachable. Scrolling down slims the bar to the address alone and scrolling up brings its controls back.
- Pages laid out at a phone-sized zoom rather than the engine's smaller default.
- Tabs can be carried to another place in the grid; the order is kept across restarts.
- Tab groups, as Safari has them: the grid shows one group at a time, and a strip along its head names them, the current one underlined and kept in the middle. An edit button in the strip's corner lists the groups to rename and delete them, with a row under the last one that makes a new group; a search button in the other corner finds a tab by title or address across every group. The menu moves the tab in front to another group, or to a new one. Groups survive a restart.
- The default group, first in the strip, can be neither renamed nor deleted.
- A preview in the grid is picked up to be carried by holding it for a second, and closed by sliding it to the left. The close button in its corner is a near-opaque disc in the highlight colour with a cross through it, so it can be seen over any page.
- Holding the new-tab button brings up the tabs closed lately, to open again; the list survives a restart.
- Five pages stay loaded, the ones read most recently; the rest reload when their tab comes back. Settings chooses 3, 5, 10 or all. After ten minutes in the background the engine is asked to trim its memory.
- A second after the application is put away, the loaded pages are put to sleep -- their scripts, timers and workers stop -- so a busy site no longer keeps the phone busy out of sight. A page playing something with sound, or in a call, keeps them awake, and they sleep five seconds after it stops. Each page wakes as it is next on the screen.
- Back, reload and stop on the navigation bar, which the address field takes over while it is being edited. Share in the menu.
- History with search, and bookmarks with edit and remove.
- Downloads through the platform transfer UI.
- Settings: home page, search engine (Qwant, Ecosia, Startpage), desktop site mode, keeping clear of the display's camera cutout (on by default), what the cover shows, clearing history, cookies and cache.
- The slim navigation bar stays opaque while a page is scrolled, so the address stays readable over a light page; the page ends above it as it does above the whole bar.
- Cover showing the number of open tabs over a monochrome field of their page previews, most recently used first, with a search action that opens a new tab with the address field already up. The preview of the tab in front is refreshed as the app is put away, so the cover shows the page as it was left. Settings chooses how much of that the cover shows: the icon alone, the tab count and the tab last read, or the count and the most recent tabs.
- Finnish translation.
