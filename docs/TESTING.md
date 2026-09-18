# Device smoke test

Run before every tag, on the Jolla Phone 2026, from the shell, never from the IDE:

    sailjail /usr/bin/harbour-salama

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
- [ ] The whole bar is opaque and the page ends above it: scroll to the foot of a long page -- a site footer, a cookie banner -- and every control there can be tapped without the bar in the way. Slim, the bar is opaque too and the page still ends above it; the host is readable over a white page.
- [ ] Scrolling down a page slims the bar to the handle and the host alone, at a smaller size; scrolling back up brings both back. The change is one smooth movement -- the bar's height, the controls fading, the host's size -- not a jump. Tapping the address, and loading a new page, both bring the whole bar back. The page does not reflow as the bar changes height.
- [ ] Settings > Avoid the screen cutout is on by default: no page content sits under the camera cutout, and the strip beside it is drawn in the page's own `theme-color` where a page declares one (github.com does; a page with no such meta leaves the strip in the application's colour). Turning it off gives that strip back to the page and puts the grid's head row under the cutout.
- [ ] A link with `target=_blank` and an in-page navigation both stay in the tab.
- [ ] Favicons: a page that declares one shows it in the tab grid and the cover, not the site's `/favicon.ico` fallback (compare two sites whose declared icon differs from their root one).
- [ ] Tab grid shows two columns of page previews that look like the pages, with favicon and title under each; tapping switches, the close button in a preview's top-right corner closes. Each preview and the highlight round the active one have rounded corners, and the picture is rounded with them rather than square inside them. Nothing square is drawn behind the active cell or a pressed one.
- [ ] A preview shows the top of what was last on screen for that tab, not the middle of the page.
- [ ] The grid's head is the strip of tab groups, the current one centred; both the strip and the first row of cells -- with its close buttons -- are clear of the screen cutout.
- [ ] The strip reads "N tabs" at first, the current one underlined in the highlight colour and its name highlighted, the names close together in small type and the row centred while it fits. The edit button in the strip's left corner opens the list of groups: the "New tab group" row under the last group makes one, tap it to make it current -- the grid shows it empty, and the button in the foot opens a tab in it. There is no pulley. A group's menu renames and deletes; the "N tabs" group has neither.
- [ ] Search tabs: type a few letters, pause, type more. The keyboard stays up and the list narrows a beat after each pause without jumping; the field never scrolls away.
- [ ] Tapping a name in the strip moves to that group; the grid shows its tabs and, pulled down, the page shows the tab last read in it. With many groups the strip scrolls and keeps the current name in the middle.
- [ ] Settings > Pages kept loaded is 5 by default. Set it to 3, open five tabs, go back to the first: it reloads from its address (a moment's blank, then the page), and `ps -o rss` on the process no longer grows with every tab visited. "All" keeps every page loaded.
- [ ] The search button in the strip's right corner lists every open tab under its group's heading; typing filters by title or address without the list jumping or the keyboard closing as results come and go; tapping a result brings that tab to the front, puts the grid away and moves the strip to its group.
- [ ] Menu > Move tab to group moves the tab in front into the group tapped, or into a new one made from the pull-down; the tab keeps its page (no reload) and the strip follows it.
- [ ] Kill the app, relaunch: the groups, their names and order, each tab's group and the current group are as they were.
- [ ] Holding a preview still for two seconds picks it up (it lifts a little); carrying it over another cell swaps them, and the order survives a restart. The tab keeps its page -- it is not reloaded. Letting go leaves the grid open; it does not jump to that tab. A shorter hold, then lifting, opens the tab as a tap does.
- [ ] Sliding a preview to the left fades it as it goes; past a third of its width, letting go closes the tab, short of that it slides back. Sliding to the right does nothing. Sliding up or down scrolls the grid.
- [ ] The close button in a preview's corner sits on a dark disc and can be seen over a white page.
- [ ] Holding the button in the foot row brings a panel up from under it listing the tabs closed lately, newest first, with title and address; tapping one opens it again with its title and returns to it, and it leaves the list. Tapping outside the panel puts it away. The list survives a restart.
- [ ] A handle is drawn along the top of the navigation bar and lights up while a drag is under way; a line in the highlight background colour runs across the very top of the screen when the grid is up, as thick as the handle.
- [ ] A cell picked up after a second and a half of holding, even when the thumb drifts a little meanwhile; the close mark in its corner is one disc in the highlight colour with a cross through it, readable over a white page.
- [ ] The drag that opens the grid can also be started just above the bar, and the screen does not judder while the finger is held.
- [ ] The grid's two rows are drawn over the cells rather than among them: the head says how many tabs, the foot carries the one button, and the first and last rows of cells can be scrolled clear of both.
- [ ] The button in the foot row opens a new tab and returns to it.
- [ ] A tab never displayed this session shows a placeholder, and fills in once visited.
- [ ] Kill the app (swipe close), relaunch: same tabs, same active tab, previews still there.
- [ ] `ls ~/.cache/io.github.muhnschein/salama` holds one PNG per previewed tab and none after closing them.
- [ ] History lists visited pages newest first; search filters; remove and clear work.
- [ ] Bookmark the page from the menu; it appears in Bookmarks; edit and remove work.
- [ ] Share sends the address to another app.
- [ ] Download a file: the transfer UI appears and the file lands in Downloads.
- [ ] Upload a photo in a web form through the platform picker (permissions check).
- [ ] Settings: the engines are Qwant, Ecosia and Startpage, Qwant first; change the home page and search engine; toggle desktop sites and confirm a site serves its desktop layout.
- [ ] Clear cookies and site data: a logged-in site asks to log in again.
- [ ] Cover shows "Salama", "Tabs" and the tab count, over a grey field of the open tabs' previews that fades in below the heading. The field fills the cover at one tab, at two, at four and at a dozen (six cells at most), and the count and the field follow opening and closing tabs.
- [ ] The field is ordered by what was read last: switch tabs, minimise, and the tab just left leads it. The order survives a restart.
- [ ] Scroll a page well down, minimise: the cover shows it scrolled, not as it was loaded. Same after stepping through a site that navigates without loading.
- [ ] Cover action is a search icon: it opens a new tab with the address field up, the url selected and the keyboard shown, from the browsing page and from Settings alike.
- [ ] Settings > Cover > Shows: "The icon alone" leaves the cover the app icon and the action, nothing else; "The tab count and the last tab" keeps the heading and draws one preview across the cover; "The tab count and the most recent tabs" is the field. Each takes effect on the cover without restarting, and the choice survives a restart.
- [ ] Rotate the phone: layout stays portrait (landscape is Phase 2).
- [ ] `journalctl -f` shows no QML warnings from `harbour-salama` during the above.
