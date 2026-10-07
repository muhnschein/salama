# Changelog

User-facing changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/); semver.
Each version's section is its GitHub release text, cut by `ci/release-notes.sh` (`.github/workflows/rpm.yml`; docs/RELEASING.md).

## [Unreleased]

### Added
- Settings > Privacy > HTTPS-Only Mode: all pages over HTTPS; non-HTTPS site asks first. Off (default): HTTPS first, HTTP fallback, like Firefox.
- Settings > Privacy > DNS over HTTPS, like Firefox for Android: Increased Protection, Max Protection or Off (default); Cloudflare, NextDNS or own provider; excepted sites.
- Settings > Privacy > Tell websites not to share & sell data: Firefox's Global Privacy Control, replaces Do not track. On if Do not track was.
- Press and hold link or picture: sheet like menu's, link named with copy button; New tab, Background tab, Share, Save link; app action for email, phone, text message, place; Open image, Save image, Copy image link. Background tab stays put, notes where link went, with Show. Saved files keep names.
- Link preview atop that sheet, like Safari: tap opens; Hide preview hides for all links until Show preview.
- Held picture lifts above sheet, max size, pinch to zoom.
- Settings > Site permissions, replaces Settings > Notifications: defaults for notifications, pop-ups, location, camera and microphone, cookies when tracking protection off; each with editable exceptions. Salama now requests location, camera, microphone from phone.
- Site details: tap page name atop menu; connection security, verifier, certificate, cipher; per-site tracking protection off; permissions.
- Site-offered search engines collected under Settings > Search > Found while browsing; tap adds and searches. Added engines show source; remove singly or all via pulley (three built-in stay).
- Salama in share sheet for links: opens in new front tab in "N tabs" group, launching Salama if needed. Plain text not offered.
- Settings > Appearance > Fixed toolbar: bar stays whole while scrolling.
- Settings > Privacy > Enable JavaScript, notes cost of turning off.
- Settings > Tracking protection ends with web engine's current limits.
- Download card above bar on originating page: name, progress, paused or failed; several: count and total progress. Shows arrival few seconds. Tap opens Downloads; swipe hides until change.
- Downloads pause, resume, retry; pre-restart ones refetchable. Row menu deletes file too; pulley clears finished; Clear list gone.
- New cover: front tab's site and title over app's bolt in faint dots, ambience colour. Downloading: ring with percentage. Playing: what plays, with art or video picture, playing/paused. No tab: bolt alone. Static; quick action and mute at foot.
- Settings > Website colours: pages with dark and light schemes follow ambience, or always light, or always dark.
- Reader view's Ambience look, now default: article like Sailfish page, ambience colours and typeface, light right-aligned title. Old Ambience now Automatic; its users get new look; Light, Sepia, Dark kept.
- Settings > History counts stored history pages, downloads, recently closed tabs, open tabs above Clear browsing data; dialog shows amount per kind.
- Settings > Start page shows half-size live new-tab preview under switches.
- Menu names page -- icon, title, padlock, host -- with copy-address button; "Start page" on start page, page actions dimmed. Downloads entry wears progress ring.
- Tab groups reorderable: drag by bars at row end. "N tabs" stays first.
- Ungroup, in tab group menu: removes group, tabs move to first group unchanged.
- Swedish translation.
- Translations into every other Sailfish OS language: Bengali, Bulgarian, Chinese (China, Hong Kong and Taiwan), Czech, Danish, Dutch, Estonian, French, German, Greek, Gujarati, Hindi, Hungarian, Italian, Kannada, Latvian, Lithuanian, Malayalam, Marathi, Norwegian Bokmål, Polish, Portuguese (Portugal and Brazil), Punjabi, Romanian, Russian, Slovak, Slovenian, Spanish, Tamil, Tatar, Telugu, Turkish, Ukrainian and Vietnamese; Firefox wording where available.

### Changed
- Downloads card now full-width bar on navigation bar, Show at end; page ends above.
- Website colours now Preferred color scheme, with sailfish-browser's description; Automatic now Match ambience.
- Avoid the screen cutout now Notch guard, sailfish-browser's modes: Automatic (default) lets cutout-aware pages use it; Forced keeps all below; Disabled none. Old on becomes Forced, off Disabled.
- Settings rows: icon or switch, never both; switch lights in icon column.
- Loading stop button: plain cross, like sailfish-browser.
- Downloads rows: progress ring at start with pause, play or retry, replacing foot line; kind icon when done. Failed in red; missing file "File not found". Removing in-progress download or clearing list stops it.
- Settings shows current value under each name (start page, search engine, reader look, cover, tracking protection level, notification sites, history). Headings: Browsing, Appearance, Privacy, Help; Privacy page now Tracking protection.
- Choices inline: start page, search engine, tracking protection level as lists, chosen lit; reader colours five painted squares, typefaces two tiles; cover quick action six rows with glyphs under cover picture.
- Name-repeating setting descriptions gone; rest add info.
- Website colours and Avoid the screen cutout get icons: moon and display, from sailfish-browser.
- Settings > Notifications lists Allowed and Blocked; switch reads Sites can ask; Forget this site removes one.
- Clear browsing data on history page now button.
- Menu: opaque sheet; page's five actions -- Find in page, Bookmark, Share, Desktop site, Reader view -- on discs in one row, lit when on; browser's four below line.
- Grid: front tab framed by thin rounded outline, not washed; smaller close discs, same tap area; target group name lights in rounded wash; search highlights typed letters; recently closed tabs on menu's sheet.
- Tab group list: square thumbnail of recent tabs per group, current framed, "N tabs" under name; new-group row has ringed plus; dialog reads New tab group, Create.
- Tutorial: shorter text, five progress dots, icon first card, check-mark last card.

### Fixed
- Menu sheet: handle hugs top edge, sheet no longer scrolls or flings, fading line parts page header from actions as it parts the two rows. Same handle on link sheet and recently closed list.
- Bar-to-grid drag no longer stutters at start: grid tracks finger from start, slim bar stays slim, page picture and grid prepared before drag.

### Removed
- Lightning cover, its flash, and cover choice in Settings: one cover now.

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
