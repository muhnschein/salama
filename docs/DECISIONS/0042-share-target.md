# 0042 — Links shared from other applications open in a new tab

## Context
Salama cannot be the system's browser: Sailfish OS hands every link to sailfish-browser,
and SCOPE.md §3 rules out registering as an `http(s)` handler. The share sheet is the way
left. An application offers itself there in its own desktop file — `X-Share-Methods`, and a
group per method naming its description and the MIME types it takes — and the sheet calls
it over D-Bus: `share(a{sv})` on the interface `org.sailfishos.share`, object
`/share/<method>`, at the service named for its Sailjail `OrganizationName.ApplicationName`,
which `ExecDBus` lets the system start it under.

Sailfish.Share's `ShareProvider` answers that call from QML, but accepts a resource only as
a file or as a name with its data. A link shared from a browser — sailfish-browser's, and
this browser's own `ShareAction` (`BrowserMenu.qml`) — is `{type, status, linkTitle}`, the
address being the status, and `ShareProvider` drops it. harbour-nextmarks found this with
`dbus-monitor` on a device and answers the call in C++ instead (`src/sharereceiver.cpp`).

## Decision
One share method, `link`, whose only capability is **`text/x-url`**: the sheet matches on
MIME type alone, so Salama is offered for links and never for plain text — not even text
that holds an address, which the sheet cannot look inside. The user asked for exactly this,
or nothing.

`ShareReceiver` (`src/share/`) answers the call itself, through `QtDBus` (on Harbour's
library list): an adaptor naming the interface, the object registered before the name is
claimed, both before the window is loaded, because the sheet gives up on a slow answer
when it had to start the application. It reads the address from the status, or from the
data for a sender shaped as `ShareProvider` expects, and passes it on only as an `http` or
`https` address with a host; anything else is dropped. A link that arrives before the window
is ready is held until it says it is.

The link opens in a **new tab in the default group** — the "N tabs" one — which comes to the
front with it, whatever group was being read (`TabModel::newTabInDefaultGroup`); whatever
page was over the browsing page is put away first, as for the cover's quick action.

## Consequences
Salama is offered in its own share sheet too: the platform has no way to leave the sender
out. Accepted — it is one more row, and choosing it opens the page again in a new tab.

The D-Bus call cannot be made on a host; the reading of it is unit-tested, and the call
itself is on the device checklist (`docs/TESTING.md`), as are both a cold and a warm start
from the share sheet.
