# Device smoke test

Run before every tag, on the Jolla Phone 2026, from the shell, never from the IDE:

    sailjail /usr/bin/harbour-tuuli

Install the RPM that `sfdk check -s harbour` accepted. Each line must pass with no
workaround; a failure blocks the tag.

## Checklist

- [ ] Launches under Sailjail; first run shows the home page in one tab.
- [ ] Page text is the size a phone browser lays it out at, not desktop-small.
- [ ] Opening and closing the keyboard does not stretch the page; the bar comes up with the keyboard.
- [ ] The bar shows the host only -- "bellard.org", not "https://www.bellard.org/" -- with the port kept when there is one.
- [ ] Tapping the address turns it into a field in place showing the whole url again, with its text on the same line the host was on; typing a host opens it over https, typing words searches with the selected engine.
- [ ] With the field up: tapping the page, or dismissing the keyboard, puts the bar back to the host. Back, reload and menu still work, and the bar can still be dragged, while the field is up.
- [ ] A site with a bad certificate (expired.badssl.com) draws a red warning glyph left of the host; a plain http site draws none; neither shows while the address is being edited.
- [ ] Scrolling a page down takes the navigation bar off the bottom, so a button at the very foot of a page can be reached; scrolling back up brings the bar in again, and loading a new page brings it in too.
- [ ] The address and the warning beside it sit centred in the bar; no port number is shown.
- [ ] Back and reload on the bar act on the current page; reload becomes stop while a page is loading; progress shows on the bar while loading; back is dimmed when there is nowhere to go back to.
- [ ] The bar carries back, the address, reload/stop and the menu, and the address is centred on the screen. Tapping the address leaves the menu alone on it and the field spans the rest of the bar, with the text starting close to its edges.
- [ ] Dragging the navigation bar upwards pulls the tab grid up from under the page, from anywhere along the bar including over the icons; tapping instead does not.
- [ ] The grid follows the finger while it moves, and a drag that stops short of the threshold springs back. The movement is vertical, never sideways.
- [ ] Dragging the grid downwards past its top brings the page back the same way; a short pull springs back to the grid.
- [ ] Menu > Tabs opens the same grid without the gesture.
- [ ] The bar is opaque and the page ends above it: scroll to the foot of a long page -- a site footer, a cookie banner -- and every control there can be tapped without the bar in the way, in both of the bar's heights.
- [ ] Scrolling down a page slims the bar to the handle and the host alone, at a smaller size; scrolling back up brings its controls back. The change is one smooth movement -- the bar's height, the controls fading, the host's size -- not a jump. Tapping the address, and loading a new page, both bring the whole bar back. The page does not reflow as the bar changes height.
- [ ] Settings > Avoid the screen cutout is on by default: no page content sits under the camera cutout, and the strip beside it is drawn in the page's own colour where a page has one. Turning it off gives that strip back to the page and puts the grid's head row under the cutout.
- [ ] A link with `target=_blank` and an in-page navigation both stay in the tab.
- [ ] Tab grid shows two columns of page previews that look like the pages, with favicon and title under each; tapping switches, the close button in a preview's top-right corner closes. Each preview and the highlight round the active one have rounded corners, and the picture is rounded with them rather than square inside them. Nothing square is drawn behind the active cell or a pressed one.
- [ ] A preview shows the top of what was last on screen for that tab, not the middle of the page.
- [ ] The grid's head says how many tabs there are, and both that text and the first row of cells -- with its close buttons -- are clear of the screen cutout.
- [ ] Dragging a preview sideways picks it up; carrying it over another cell swaps them, and the order survives a restart. The tab keeps its page -- it is not reloaded. Letting go leaves the grid open; it does not jump to that tab.
- [ ] A handle is drawn along the top of the navigation bar and along the top of the grid's head row; the bar's lights up while a drag is under way.
- [ ] The drag that opens the grid can also be started just above the bar, and the screen does not judder while the finger is held.
- [ ] The grid's two rows are drawn over the cells rather than among them: the head says how many tabs, the foot carries the one button, and the first and last rows of cells can be scrolled clear of both.
- [ ] The button in the foot row opens a new tab and returns to it.
- [ ] A tab never displayed this session shows a placeholder, and fills in once visited.
- [ ] New private tab shows "Private tab" when the address is tapped; its pages do not appear in History, and its grid cell never shows a preview.
- [ ] Kill the app (swipe close), relaunch: same tabs, same active tab, private tabs gone, previews still there.
- [ ] `ls ~/.cache/io.github.muhnschein/tuuli` holds one PNG per previewed tab and none after closing them.
- [ ] History lists visited pages newest first; search filters; remove and clear work.
- [ ] Bookmark the page from the menu; it appears in Bookmarks; edit and remove work.
- [ ] Share sends the address to another app.
- [ ] Download a file: the transfer UI appears and the file lands in Downloads.
- [ ] Upload a photo in a web form through the platform picker (permissions check).
- [ ] Settings: the engines are Qwant, Ecosia and Startpage, Qwant first; change the home page and search engine; toggle desktop sites and confirm a site serves its desktop layout.
- [ ] Clear cookies and site data: a logged-in site asks to log in again.
- [ ] Cover shows the current tab's title and favicon; cover action opens a new tab.
- [ ] Rotate the phone: layout stays portrait (landscape is Phase 2).
- [ ] `journalctl -f` shows no QML warnings from `harbour-tuuli` during the above.
