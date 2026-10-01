# 0040 — Site details, from the head of the menu sheet

## Context
The menu sheet's head (0021) names the page and shows the padlock the bar draws, and
does nothing else with it. sailfish-browser's padlock opens a certificate page
(`CertificateInfo.qml`) with the verdict, the issuer, the cipher suite, and a way to the
site's permissions; Firefox's opens a panel with the connection, tracking protection for
the site and its permissions. Here the sheet's head is where a site is named, and the
natural thing to tap. The engine's verdict is `view.security`, a `QMozSecurity`
(qtmozembed), which the bar and the head read through bindings that tolerate its being
absent (0011).

## Decision
**A tap on the head** opens `SiteDetailsPage`; a chevron after the title says so, and the
copy button keeps its own tap. On the start page there is no site, and neither chevron nor
tap. `BrowserMenu.openSiteDetails()` gives the page the address, the title and the view.

**Connection.** A padlock and "Connection is secure" with "Verified by *issuer*", or a
warning in the error colour and "Connection is not secure" with the reason the engine
names, most to the point first -- expired or not yet valid, another site's, not trusted --
and "Do not enter personal data, passwords, card details on this site". A plain http
address is not secure and has no reason and no certificate to describe. Under a
"Connection" heading, as `DetailItem`s: issued to, verified by, valid until, protocol
(`protocolVersion` as SSL 3.0 to TLS 1.3) and cipher suite; a line the engine has nothing
for, or all of them, is not drawn. The words are sailfish-browser's.

**Tracking protection for the site** is a switch. It adds the `trackingprotection`
permission of the site's origin or takes it away (0039), and loads the page again, which
is when the engine applies it. While on, its words are what to do about it: "If something
looks broken on this site, try turning this off."; otherwise that it is off for the site. Off in Settings it is off for every site: the switch is dimmed
and says "Off in Settings". While on, and when `blockedTrackingContent` is true, the page
says trackers were blocked.

**Permissions**, a row for each kind with what the site was given -- "Allowed",
"Blocked", "Always ask" -- or, with none, "Follow default: " and what the default is, so
that the line never reads as a choice of the site's own. A tap offers the same: Allow,
Block, Always ask for the kinds a page asks for (notifications, location, camera,
microphone), and "Follow default: Block" or whatever the default is. Always ask is the
engine's own record for it, nsIPermissionManager's PROMPT_ACTION, 3: the site is asked
even while the default blocks every other, which following an asking default is not.
The cookies row is shown while tracking protection is off, for every site or this one, or
when the site has an exception to cookies. "Clear site permissions", in the pull-down menu
and only while the site has a choice of its own, removes every exception of the site,
tracking protection's too, and loads the page again if that was one. An
address that is not http or https has no origin and none of these sections.

## Consequences
`view.security` is the stub's in the load tests, which gains the properties read
(`tst_qmlload`: `siteDetailsConnection`, `siteDetailsTrackingProtection`,
`siteDetailsPermissions`, `menuHeadOpensSiteDetails`). The certificate's own details
(its chain, fingerprints) are not shown: sailfish-browser's "Details" button has no
counterpart.
Whether the engine applies the allow list on reload, and what `QMozSecurity` holds for a
page that has none, are device checks (`TESTING.md`). A second `Verified by` appears in the
hero and the list: the engine gives one issuer name, and both use it.
