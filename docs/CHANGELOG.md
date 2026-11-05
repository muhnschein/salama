# Changelog

User-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); semver.
Each version's section is its GitHub release text, cut by `ci/release-notes.sh` (`.github/workflows/rpm.yml`; docs/RELEASING.md).

## [Unreleased]

## [0.9.0] - 2026-11-05

### Features
- Privacy on by default: HTTPS-Only Mode, DNS over HTTPS (Quad9 by default, or Cloudflare, NextDNS, your own) and Global Privacy Control, which replaces Do not track. Per-site permissions, a JavaScript switch, and site details with the certificate authority named (#26, #36, #65, #67, #68)
- Settings reworked into seven pages, with privacy and security on one of them (#20, #66)
- Long-press sheet for links and pictures, with link preview and pinch zoom (#35). Holding the back button lists tab history (#57)
- Downloads: pause, resume and retry, progress notices, search, grouped by date (#25, #51). History is grouped by date, shows icons and highlights matches (#51)
- New cover showing site, download progress and what's playing. The lightning cover is gone (#21)
- Menu sheet, tab groups and tutorial overhauled (#20, #52). Start page can show one or two rows of tiles (#47). Share-sheet target for links, and site-offered search engines (#26)
- Swedish, then all 39 Sailfish OS languages (#30, #32)

### Fixes
- Menu sheet handle and scrolling (#42), keyboard focus in the History and bookmark picker search fields (#50)
- App could hang on close and then fail to start again (#58)
- Duplicate history rows, and English plurals reading "3 page(s)"

### Performance
- Grid drag no longer stutters (#34). Smoother bar taps, lists, typing and tab grid (#64). Less background work: lighter saves, smaller page pictures

### Build
- Reproducible release RPMs: anyone can rebuild a release from its tag and get the same file (#56, docs/VERIFYING.md)

## [0.8.0] - 2026-09-26

### Fixed
- English and untranslated languages read "1 page", "3 pages", not "3 page(s)": English catalog never loaded.
- Upload picker lists phone's images, videos, music, documents, not empty pages. App now holds Sailjail's `MediaIndexing` permission (picker search), plus `Videos` and `Music` (their folders).
- Player controls just above navigation bar work: tap or sideways/down drag (seek bar) goes to page; upward drag still opens grid.
- Pages play sound: app now holds Sailjail's `Audio` permission.
- Grid pulls back to page from anywhere again, preview and head row included; long grid scrolls from drag on preview. Held preview released on vertical move, like list item.

### Changed
- Long tab group: drag down grid's head row ("Search tabs") returns page from any scroll position; grid keeps position. Tap still opens search.
- Cover's search opens empty address bar for new tab listing bookmarks, not home page; no tab until choice made.
- Settings: main page linking start page, search, reader view, cover, privacy, history; cutout switch under Appearance. Request desktop sites and Pages kept loaded removed: menu's Desktop version per page; five pages loaded, like Jolla's browser. Four clear buttons (history, cookies and site data, cache, open tabs) now one Clear browsing data dialog on history page: per-kind switches, time range, one remorse.
- Menu sheet: two rows, This page and Browser; New tab removed (grid's plus or cover's search).
- Grid's "Search tabs" matches every typed word in title or address, not whole string.
- Quieter tab grid: no favicon or title under previews; active preview faint wash only, no border; close button dark/light disc per ambience, opaque cross; opaque head and foot rows.
- New app icon: pale pink lightning bolt on plum-to-navy disc, mauve frame.
- Grid preview pickup after 1 s hold, not 1.5 s.
- Private tabs and private group removed: platform gives Harbour apps no device-lock or fingerprint gate. Existing private tabs dropped on first start.
- App renamed salama (was tuuli): package `harbour-salama`, Sailjail application name `salama`. New data directory; tuuli tabs, bookmarks, history, settings don't carry over.

### Added
- Short tutorial, like Sailfish's Tutorial: tap address bar, open/close menu, drag bar up to tabs, close/move tab and move to other group, pull grid down. Silica animated hints, waits for each gesture; no real tabs touched. First start: card with icon and name, start or skip. Settings > Help > Tutorial replays.
- Web notifications, like Firefox: after page tap, "Allow *site* to send notifications?" -- Allow, Always block, Not now. Shown in phone's notifications with site name and icon; tap opens tab; gone with page or tab. Allowed site keeps running out of sight. Settings > Notifications lists allowed/blocked sites, blocks new requests.
- Start page, like Firefox's home, replaces home page: every new tab and first start. Tiles for top sites and first bookmarks, rows for recent pages; no searches. Back from opened page returns to it. Settings > Start page toggles sections or blank; home page address settings gone.
- Address bar suggestions from open tabs (all groups), bookmarks, history, downloads; rows above bar to go to address or search web. Short, bottom-up, Firefox ranking: one row per page; past picks first, then site-name prefix, then frequency and recency; files last. Site icon or initial, typed words bold. Open tab: "Switch to tab" in ambience colour, fronts it in its group; finished download opens. Hide keyboard or drag list for more; tap bare glass ends edit. Settings > Search picks sources.
- Settings > History: remember visits; clear history, downloads list, recently closed tabs on close; Clear browsing data range: last hour, two, four, today, everything.
- Settings > Reader view: live article preview per choice.
- Cover quick action in Settings > Cover: none, search, bookmarks, one bookmark, downloads, history. Explains single slot (other is media control); previews idle and playing cover. Bookmark from searchable list, one of eight glyphs; fronts its tab, any group, else new tab. Survives rename or re-add; if gone, opens bookmarks.
- Sound-playing tab shows mute: grid preview foot; navigation bar left of host, ambience colour; cover beside quick action for front tab. Speaker struck through when muted, paused or behind front. Mute pauses page; unmute resumes; unmuting background tab fronts it. Mute persists across pages until unmuted or closed. Only for media page's own scripts reach, not cross-site embeds.
- Only front tab plays: leaving pauses, returning resumes, YouTube mobile included. Engine silences background tabs. Front tab starting playback pauses others.
- Out of sight, playing page keeps sound, videos hidden; no decoding unseen frames.
- Firefox's tracking protection, Settings > Privacy. Standard (default): per-site cookies; Strict: more anti-fingerprinting, trimmed referrer. With ESR 153 engine (fetches lists): Standard blocks known fingerprinters, cryptominers; Strict blocks all known trackers, strips tracking parameters, clears bounce trackers' data. Off: engine default.
- Reader view, like Firefox, from menu's *This page* row: articles as title, byline, text, read-time estimate, Firefox's reader styles. Dimmed on non-articles, lit when on; tap again or back exits. Address, history, bookmarks keep article URL. Settings > Reader view: colours (ambience's, light, sepia, dark), typeface, size; applied live.
- Multi-tab browsing, grid of previews, active in square wash; all survive restart. Grid head and foot: opaque ambience glass, keyboard pattern.
- Drag navigation bar up for tab grid; drag grid down past top, from anywhere, or tap preview to return. Handle on bar, line atop grid mark grab spots.
- Navigation bar shows host; tap edits full url in place; addresses or search with configurable engine. Scroll down slims bar; scroll up or tap restores; tap whole bar edits.
- Red address warning on broken TLS connection; page ends above navigation bar, foot always reachable. Scroll down slims bar; up restores controls.
- Phone-sized page zoom, not engine's smaller default.
- Tabs reorderable in grid; order kept across restarts.
- Tab groups, like Safari: grid shows one group; foot strip names them, current underlined and centred, edges fade when more. Edit button, foot's right corner: rename, delete, new-group row. "Search tabs" field atop grid matches every typed word, title or address, across groups. Groups survive restart.
- Preview dropped on group name in strip moves tab there; name lights on hover. Moving front tab takes grid along.
- Default group, first in strip: no rename or delete.
- Grid preview: 1 s hold to carry, slide left to close. Corner close button: faint dark/light disc per ambience, opaque under finger.
- New-tab button, grid foot's left corner; hold for recently closed tabs; list survives restart.
- Five most recent pages stay loaded; rest reload on return. Settings: 3, 5, 10 or all. After ten minutes backgrounded, engine trims memory.
- One second after app put away, pages sleep -- scripts, timers, workers stop. Sound-playing or in-call page stays awake, sleeps five seconds after stopping. Page wakes when shown.
- Back, reload, stop on navigation bar; address field replaces them while editing.
- Menu: icon sheet rising from navigation bar, two rows: search on page, bookmark, share, desktop version; bookmarks, history, downloads, settings. Tap outside or pull down dismisses.
- Search on page: field over navigation bar; highlights and scrolls to match, previous/next arrows, error colour on no match.
- Per-page desktop version from menu; Settings governs rest.
- History with search; bookmarks with edit and remove.
- Downloads to Downloads/Salama without prompt, listed from menu: newest first, progress, outcome; tap opens finished. List survives restart; clearing keeps files.
- Settings: start page content, search engine (Qwant, Ecosia, Startpage), desktop site mode, avoid display's camera cutout (on by default), cover content and quick action, clear history, cookies and site data, cache, open tabs.
- Slim navigation bar opaque while scrolling, readable over light pages; page ends above it.
- Cover: lightning bolt in ambience colour, sheet-lightning flash on each view; quick action (search unless set otherwise) opens address bar for new tab. Option: open-tab count over grey picture of last front tab, refreshed on app put away.
- Finnish translation.
