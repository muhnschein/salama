# Changelog

All notable, user-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow semantic versioning.

## [Unreleased]

### Added
- Multi-tab browsing with a grid of page previews; tabs, the active tab and the previews survive restarts.
- Dragging the navigation bar upwards pulls the tab grid up from under the page, and dragging the grid down past its top puts the page back; tapping a preview does the same. The menu reaches the grid without the gesture.
- Address shown in the navigation bar as the host alone, tapped to edit the whole url in place, opening addresses or searching with a configurable engine.
- A red warning on the address when the engine reports a broken TLS connection, and a navigation bar that gets out of the way when a page is scrolled down.
- Pages laid out at a phone-sized zoom rather than the engine's smaller default.
- Tabs can be carried to another place in the grid; the order is kept across restarts.
- Back, reload and stop in the menu, and share.
- History with search, and bookmarks with edit and remove.
- Private tabs that leave no history and keep no cookies.
- Downloads through the platform transfer UI.
- Settings: home page, search engine (Qwant, Ecosia, Startpage), desktop site mode, clearing history, cookies and cache.
- Cover showing the current tab's title and icon, with a new-tab action.
- Finnish translation.
