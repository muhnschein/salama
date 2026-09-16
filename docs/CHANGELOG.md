# Changelog

All notable, user-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow semantic versioning.

## [Unreleased]

### Added
- Multi-tab browsing with a grid of page previews; tabs, the active tab and the previews survive restarts.
- Dragging the navigation bar upwards pulls the tab grid up from under the page, and dragging the grid down past its top puts the page back; tapping a preview does the same. A handle on each of those edges says where to take hold. The menu reaches the grid without the gesture.
- Address shown in the navigation bar as the host alone, tapped to edit the whole url in place, opening addresses or searching with a configurable engine. Scrolling down slims the bar to the address alone and lets the page run behind it; scrolling up brings it back whole.
- A red warning on the address when the engine reports a broken TLS connection, and a navigation bar the page ends above rather than running behind, so the foot of a page is always reachable. Scrolling down slims the bar to the address alone and scrolling up brings its controls back.
- Pages laid out at a phone-sized zoom rather than the engine's smaller default.
- Tabs can be carried to another place in the grid; the order is kept across restarts.
- Back, reload and stop on the navigation bar, which the address field takes over while it is being edited. Share in the menu.
- History with search, and bookmarks with edit and remove.
- Private tabs that leave no history and keep no cookies.
- Downloads through the platform transfer UI.
- Settings: home page, search engine (Qwant, Ecosia, Startpage), desktop site mode, keeping clear of the display's camera cutout (on by default), clearing history, cookies and cache.
- Cover showing the number of open tabs over a monochrome field of their page previews, most recently used first, with a search action that opens a new tab with the address field already up. The preview of the tab in front is refreshed as the app is put away, so the cover shows the page as it was left.
- Finnish translation.
