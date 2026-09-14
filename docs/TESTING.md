# Device smoke test

Run before every tag, on the Jolla Phone 2026, from the shell, never from the IDE:

    sailjail /usr/bin/harbour-tuuli

Install the RPM that `sfdk check -s harbour` accepted. Each line must pass with no
workaround; a failure blocks the tag.

## Checklist

- [ ] Launches under Sailjail; first run shows the home page in one tab.
- [ ] Tapping the address turns it into a field in place; typing a host opens it over https, typing words searches with the selected engine.
- [ ] Back, reload and stop act on the current page; progress shows while loading.
- [ ] Dragging the navigation bar upwards opens the tab grid, from anywhere along the bar including over the icons; tapping instead does not.
- [ ] Menu > Tabs opens the same grid without the gesture.
- [ ] A link with `target=_blank` and an in-page navigation both stay in the tab.
- [ ] Tab grid shows two columns of page previews that look like the pages, with favicon and title under each; tapping switches, the close button in a preview's top-right corner closes.
- [ ] Grid header names the active tab and counts the rest; its pulley returns to that tab.
- [ ] A tab never displayed this session shows a placeholder, and fills in once visited.
- [ ] New private tab shows "Private tab" when the address is tapped; its pages do not appear in History, and its grid cell never shows a preview.
- [ ] Kill the app (swipe close), relaunch: same tabs, same active tab, private tabs gone, previews still there.
- [ ] `ls ~/.cache/io.github.muhnschein/tuuli` holds one PNG per previewed tab and none after closing them.
- [ ] History lists visited pages newest first; search filters; remove and clear work.
- [ ] Bookmark the page from the menu; it appears in Bookmarks; edit and remove work.
- [ ] Share sends the address to another app.
- [ ] Download a file: the transfer UI appears and the file lands in Downloads.
- [ ] Upload a photo in a web form through the platform picker (permissions check).
- [ ] Settings: change the home page and search engine; toggle desktop sites and confirm a site serves its desktop layout.
- [ ] Clear cookies and site data: a logged-in site asks to log in again.
- [ ] Cover shows the current tab's title and favicon; cover action opens a new tab.
- [ ] Rotate the phone: layout stays portrait (landscape is Phase 2).
- [ ] `journalctl -f` shows no QML warnings from `harbour-tuuli` during the above.
