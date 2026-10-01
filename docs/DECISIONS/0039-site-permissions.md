# 0039 — Site permissions: the engine's, with defaults here

## Context
Settings had one permission, notifications (0033): a switch for what a site not yet
decided on may do, and the sites decided on, which are the engine's. The engine keeps
more of them. Gecko's permission manager holds a record of allow or deny for a site's
origin for every kind of thing a page may ask for, and embedlite-components'
`ContentPermissionManager.js` takes any kind by name (`add`, `remove`, `get-all`); the
platform's `PermissionModel` knows five, which sailfish-browser lists on pages of its own
(`PermissionPage.qml`, `PermissionExceptionsPage.qml`), and its certificate page links to
a page of the site's. Firefox has them under Settings > Privacy > Permissions and in a
panel for the site beside its address.

What a site may do unless it has a record is a preference of the engine, not a record.
Firefox's pop-up blocker is `dom.disable_open_during_load`; the rest are
`permissions.default.<name>`, 0 to ask and 2 to refuse. Cookies are
`network.cookie.cookieBehavior`, which tracking protection (0023) already writes.

## Decision
**The exceptions are the engine's, read and written as notifications' are.**
`SitePermissions` is one list of every record of seven kinds -- notifications, pop-ups,
cookies, location, camera, microphone and tracking protection -- as `{kind, origin,
allowed}`, with the counts a page asks (by kind, by site, sites with any), `set`,
`remove`, `removeAll` of a kind and `removeAllForOrigin`, each a message for the engine
and a change in the list. What reads the engine's answers and builds its messages
(`engine/EnginePermissions`) is shared with `NotificationPermissions`, which keeps its
own list because `WebNotifications` asks it on every request; `Core` has each take in
what the other decided, without telling the engine twice. `SiteExceptions` is the view
of one kind: the allowed first and the blocked after them, by host, as 0036's list.
Records for the session -- the platform's refusals (0033) -- are not the reader's and are
left out, as are capabilities that are not allow or deny.

**Names that are not certain.** A location is `geolocation` in what the platform's prompt
writes and sailfish-browser lists, and Gecko's own front end calls it `geo`, in its
default preference too. Neither can be checked from the host, so both are written, both
read, and both default preferences given. Tracking protection is Gecko's content blocking
allow list, the `trackingprotection` permission: *allowed* means off for the site.

**Defaults are `SitePermissionSettings`**, applied by `EnginePreferences.qml` as 0023's
are: pop-ups Block (Allow), location, camera and microphone Ask (Block), cookies Block
cross-site (Allow all, Block all). Notifications keep `PrivacySettings`. **Cookies are two
settings' preference**: while tracking protection is Standard or Strict its level's, and
while it is Off the reader's choice, so `trackingProtectionPreferences(level, cookies)`
takes the choice and only Off uses it. Off was the engine's default, every cookie; it is
now cross-site blocked until the reader changes it.

**Settings > Site permissions** (`SitePermissionsPage`) has a row for each kind with
how it is set and how many sites are an exception; a tap opens a context menu with the
choices of the default and "Show exceptions". Notifications go to their own page. The
cookies row is there only while tracking protection is Off, or a site has a cookie
exception, since the choice does nothing else. A section "Turned off for some sites" lists
tracking protection's. `SiteExceptionsPage` is one page for every kind: an "Add a site"
dialog (an address that begins with `http://` or `https://`, as sailfish-browser's),
"Remove all exceptions" after a remorse, the sites under Allowed and Blocked with a
menu to switch or remove each. For tracking protection there is one heading and nothing
to switch. The main Settings row counts `SitePermissions.exceptionSiteCount`.

## Consequences
`Location`, `Camera` and `Microphone` join the Sailjail permissions (`HARBOUR.md`):
without them the choices would be offered for what the sandbox refuses anyway. Which
names the engine reads, whether a default preference is honoured by this embedding's
prompt, and whether the allow list takes effect on reload are device checks
(`TESTING.md`). `tst_sitepermissions` covers the model and the defaults,
`tst_enginemessages` the preferences, `tst_qmlload` the pages.
