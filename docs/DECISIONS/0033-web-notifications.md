# 0033 — Web notifications, as Firefox has them, over the platform's

## Context
Pages ask to notify through the Notifications API: `Notification.requestPermission()`,
`new Notification(title, options)`, and a service worker registration's
`showNotification()`. Sailfish's Gecko has the API and cannot carry it out. Its alerts
service has no system backend on this platform and falls back to opening a XUL window,
which embedlite has no way to open; and the platform's WebView refuses every permission
request but a location's (`import/popups/PopupOpener.qml` in
sailfish-components-webview), which embedlite-components' `ContentPermissionPrompt.js`
writes down as a refusal for the session. sailfish-browser has no notifications either.

Firefox asks "Allow *site* to send notifications?", with Allow, Always Block and, where
enabled, Not Now, and only while the page handles something the reader did
(`dom.webnotifications.requireuserinteraction`). The answer is the `desktop-notification`
permission of the site's origin in Gecko's permission manager, listed in its settings
with a switch to block new requests. A notification names its site, is closed when its
page goes (`Notification::Observe`), and a tap on one brings its tab to the front; Firefox
for Android always opens on the tab.

## Decision
**The page's Notification is the browser's.** A script run in every page
(`WebNotifications::pageScript()`, run as each document arrives and again once it has
loaded) puts a Notification of its own in the page's place, and `showNotification()` and
`getNotifications()` on a service worker registration. It reads the permission from
Gecko's own `Notification.permission`, and asks the browser where Gecko would ask its
alerts service. What it says goes out as an event on the window.

**A frame script carries it to the application.** 0026 turned frame scripts down as
privileged code bound to Gecko's internals. This one binds to none: it listens for the
page's event and sends it on with `sendAsyncMessage`, adding the two things the page
cannot forge -- its origin, and the permission Gecko holds for it, read through the
window's own Notification -- and says when the document goes (`pagehide`). That is the
message manager's API, which embedlite's own helpers are written in, which the platform's
WebView uses for its text zoom (`TextZoom.js`) and sailfish-browser for its page
metadata (`PageMetadata.js`); with no such script, nothing a page does reaches the
application unasked. It is a string of `WebNotifications`', loaded as a `data:` url when
the engine has made the view. Answers go back as scripts run in the page, as the other
scripts here do.

**The permission is Gecko's, kept where Firefox and sailfish-browser keep it.**
`NotificationPermissions` writes and reads it over embedlite-components'
`ContentPermissionManager.js` topics, as the platform's `PermissionManager` does. Asked,
the browser pushes a dialog with Firefox's question: Allow and Always block are kept for
good; backing out is Not now, which refuses the page without asking again until it is
loaded again. It asks only for the tab in front, while it is on the screen with nothing
over it, and only within five seconds of a touch or a key the engine delivered -- Gecko's
transient activation, which the page script counts, the engine's own count not being
readable before Firefox 120. When a page asked the engine itself before the page script
was in place, the platform's refusal for the session is taken back for the page's own
site. Settings > Notifications lists the sites, each allowed or blocked, with a way to
change it or remove it, and Firefox's *Block new requests*, the engine's
`permissions.default.desktop-notification`.

**What is shown is the platform's.** `components/NotificationCenter.qml` makes a
Nemo.Notifications notification of each, with the page's title and text and the site's
host under them. A tag shows a notification in place of the site's last with the same
tag, as Gecko names an alert by origin and tag. The page's icon is drawn by the page --
from its own site, or one that allows it -- no larger than the platform shows one, and
handed over as a PNG, kept in the cache while shown. A tap brings the tab to the front,
over whatever is open, and the browser with it; the page hears `click`, then `close`.
Swiped away, closed by the page, or with the page gone, the tab closed or the browser
quit, a notification goes and the page hears `close`. What a browser that did not quit
left behind is closed as the next starts.

**A page of a site allowed to notify is not put to sleep out of sight** (amending 0020):
it could send nothing asleep. Its view goes inactive with the rest.

## Consequences
Notifications come while the site's page is loaded in a tab -- one of the five pages kept
loaded (0016) -- and the browser runs. There is no Push API: nothing wakes a page that is
not loaded. A service worker's own `showNotification()` is out of reach, and a tap on a
notification a page showed for its worker brings the tab to the front and nothing more:
`notificationclick` is the worker's. A page that keeps its first `Notification` before
the page script replaces it shows nothing through it. Frames keep Gecko's Notification:
one from another site is refused by Gecko, one from the same site meets the platform's
refusal, taken back.

A site allowed to notify keeps its page running out of sight, as in Firefox; blocking or
removing it in Settings puts it to sleep with the rest. `preventDefault()` in a page's
click handler does not keep the browser in the background, as it does not on Android.
Actions, badges, images, vibration and `renotify` are not shown; `silent` and
`requireInteraction` are read and change nothing. An icon from another site that does not
allow it is left out, as is one that takes more than three seconds.

`tst_webnotifications` runs both scripts over the parts of the DOM and the message
manager they use; `tst_qmlload` drives the prompt, the settings page and the
notifications. Whether the engine loads a frame script from a `data:` url, what the
platform draws of a notification, and a tap on one reaching the browser in the
background are on the device checklist (`docs/TESTING.md`).
