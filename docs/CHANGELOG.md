# Changelog

All notable, user-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versions follow semantic versioning.

## [Unreleased]

### Fixed
- A player's controls just above the navigation bar work: a tap there, or a drag sideways or down -- along a seek bar -- goes to the page, while a drag upwards from there still opens the grid.
- Pages play sound. The application now holds Sailjail's `Audio` permission, without which the platform keeps it out of the sound system altogether.
- The grid can be pulled back to the page from anywhere on it again, a preview or the head row included, and a grid longer than the screen scrolls from a drag begun on a preview. A preview being held is let go when the finger moves up or down, as a held list item is.

### Changed
- The tab grid is quieter: nothing under the previews -- no favicon, no title -- so the pictures have the room; the active preview is marked by its faint wash alone, without the border in the highlight colour; the close button is a dark or light disc, as the ambience is, rather than one in its colour, with an opaque cross; and the rows along the head and foot are opaque.
- A new application icon: a pale pink lightning bolt on a plum-to-navy disc in a mauve frame.
- A preview in the grid is picked up after a second of holding rather than a second and a half.
- Private tabs are gone, and the private group with them: the platform offers a Harbour application no way to put a device-lock or fingerprint gate on them, and an unlocked "Private" group would promise what it cannot keep. A database that still has private tabs loses them on first start.
- The application is now called salama (it was tuuli): package `harbour-salama`, Sailjail application name `salama`. The data directory changes with the name, so tabs, bookmarks, history and settings from a tuuli build do not carry over.

### Added
- A tab whose page plays something with sound says so, with a mute: centred at the foot of its preview in the grid, where the picture fades out under it as a cover's quick actions sit on the cover; in the navigation bar left of the host, in the ambience's colour, with the host kept in the middle of the bar; and on the cover as a second action beside the search, while the tab in front plays. The speaker shows whether the tab is heard: struck through while it is muted, paused, or behind the tab in front. Muting pauses the page as well, and unmuting plays again what that paused; unmuting a tab behind the front brings it to the front. A muted tab stays muted across its pages until it is unmuted or closed. Only media the page's own scripts can reach has the mute: not a player embedded from another site.
- What plays is the tab in front's: a tab left while it plays is paused as it is left, and plays again when it is back in front, YouTube's mobile site included, which pauses itself on being hidden. Sailfish's engine silences every tab behind the front, so music in one tab does not go on while another is read. When the tab in front starts playing, every other tab that plays is paused.
- Out of sight, a page that plays goes on playing its sound with its videos hidden, so the engine stops decoding pictures nobody sees.
- Firefox's tracking protection, in Settings > Privacy. Standard, the default, keeps each site's cookies to that site; Strict also hides more of what tells one device from another and shortens the address sent on to other sites. With the ESR 153 engine, which fetches the lists, Standard also blocks known fingerprinters and cryptominers, and Strict every known tracker, strips tracking parameters from links and clears bounce trackers' data. Off leaves the engine as it was.
- Reader view, as Firefox has it, from the menu's *This page* row: a page that reads as an article is shown as its title, byline and text alone, with an estimate of how long it takes to read, in Firefox's reader styles. The entry is dimmed on pages that are not articles, and lit while the reader view is up; tapping it again, or back, returns to the page. The address, the history and bookmarks keep the article's own address. Settings > Reader view chooses the colours (the ambience's, light, sepia or dark), the typeface and the text size, and a reader view on the screen follows them at once.
- Multi-tab browsing with a grid of page previews, the active one in a square wash; tabs, the active tab and the previews survive restarts. The rows along the grid's head and foot are panes of the ambience's glass, patterned as the keyboard is, and opaque.
- Dragging the navigation bar upwards pulls the tab grid up from under the page, and dragging the grid down past its top, from anywhere on it, puts the page back; tapping a preview does the same. A handle on the bar's edge and a line across the top of the grid say where to take hold.
- Address shown in the navigation bar as the host alone, tapped to edit the whole url in place, opening addresses or searching with a configurable engine. Scrolling down slims the bar to the address alone; scrolling up, or a tap on the slim bar, brings it back whole, and a tap on the whole bar edits the address.
- A red warning on the address when the engine reports a broken TLS connection, and a navigation bar the page ends above rather than running behind, so the foot of a page is always reachable. Scrolling down slims the bar to the address alone and scrolling up brings its controls back.
- Pages laid out at a phone-sized zoom rather than the engine's smaller default.
- Tabs can be carried to another place in the grid; the order is kept across restarts.
- Tab groups, as Safari has them: the grid shows one group at a time, and a strip along its foot names them, centred, in type sized to sit with the search field, the current one underlined and kept in the middle, the row fading out at an end with more names past it. An edit button in the foot's right corner lists the groups to rename and delete them, with a row under the last one that makes a new group. A "Search tabs" field along the head of the grid lists the tabs whose title or address holds what is typed, across every group, in place of the previews. Groups survive a restart.
- A preview picked up and carried down onto a group's name in the strip moves its tab into that group; the name lights while the preview is over it. Carrying the tab in front takes the grid to its new group with it.
- The default group, first in the strip, can be neither renamed nor deleted.
- A preview in the grid is picked up to be carried by holding it for a second, and closed by sliding it to the left. The close button in its corner is a dark or light disc, as the ambience is, with a cross through it, faint enough not to crowd the preview and opaque under a finger.
- The new-tab button sits in the left corner of the grid's foot; holding it brings up the tabs closed lately, to open again, and the list survives a restart.
- Five pages stay loaded, the ones read most recently; the rest reload when their tab comes back. Settings chooses 3, 5, 10 or all. After ten minutes in the background the engine is asked to trim its memory.
- A second after the application is put away, the loaded pages are put to sleep -- their scripts, timers and workers stop -- so a busy site no longer keeps the phone busy out of sight. A page playing something with sound, or in a call, keeps them awake, and they sleep five seconds after it stops. Each page wakes as it is next on the screen.
- Back, reload and stop on the navigation bar, which the address field takes over while it is being edited.
- The menu is a sheet of icons that comes up from under the navigation bar, in three rows: a new tab; search on the page, bookmark it, share it, or ask for its desktop version; bookmarks, history, downloads and settings. A tap outside puts it away, and so does pulling it back down, from anywhere on it.
- Search on page: a field over the navigation bar finds text in the page, highlighting the match and scrolling to it, with arrows to the previous and next match and the field in the error colour when there is none.
- The desktop version of one page, from the menu, while the setting in Settings still decides the rest.
- History with search, and bookmarks with edit and remove.
- Downloads, saved to Downloads/Salama without asking where, and listed in the browser from the menu: newest first, with their progress while they come and how they ended after; a tap opens a finished one. The list survives a restart; clearing it leaves the files.
- Settings: home page, search engine (Qwant, Ecosia, Startpage), desktop site mode, keeping clear of the display's camera cutout (on by default), what the cover shows, clearing history, cookies and cache.
- The slim navigation bar stays opaque while a page is scrolled, so the address stays readable over a light page; the page ends above it as it does above the whole bar.
- Cover showing the number of open tabs over a monochrome field of their page previews, most recently used first, with a search action that opens a new tab with the address field already up. The preview of the tab in front is refreshed as the app is put away, so the cover shows the page as it was left. Settings chooses how much of that the cover shows: the icon alone, the tab count and the tab last read, or the count and the most recent tabs.
- Finnish translation.
