# 0011 — The bar shows the host, and draws an open padlock when TLS is broken

## Context
`https://www.bellard.org/` in a bar the width of a phone is mostly scheme and slashes.
What a person needs from a glance at the address is whose page this is, and whether the
connection to it is what it claims to be.

## Decision
While the address is **not** being edited the bar shows `Settings::displayAddress(url)`:
the host, without the scheme, without a leading `www.`, without the path. Tapping it
brings the field up with every character of the url back, so nothing is hidden from
someone who asks for it.

The host is shown as the engine reports it, **not** reduced to a registrable domain.
`docs.example.com` and `example.com` are different sites, and deciding where a site ends
needs the public suffix list — a table to carry, to update, and to be wrong about on the
day it goes stale. A port is kept, because a port is a different server. A url with no
host at all — `about:`, `data:`, `file:` — is shown exactly as it is.

When the page came over **https** and the engine is not satisfied with the connection, a
red **open** padlock is drawn to the left of the host. The verdict is Gecko's own:
`QuickMozView` exposes a `QMozSecurity` object whose `allGood` weighs certificate,
protocol and mixed content together, and reproducing that judgement here would be both
duplicated work and a second opinion to be wrong with.

Three things follow from it being *Gecko's* verdict:

* It is read through a plain property binding, not a signal handler. An engine build
  without `security` leaves the binding undefined, the padlock hidden, and the page
  loading — a missing signal handler would have been a load error and a blank screen.
* Nothing is drawn for plain `http`. That is not broken TLS, it is no TLS, and a warning
  on every unencrypted page is a warning nobody reads. (What to do about plain http at
  all is a separate question, and not this one.)
* Nothing is drawn while the address is being edited, where the field already shows the
  whole url.

The padlock is **drawn** with `Canvas` rather than taken from the theme: the platform icon
set has no "this lock is open and that is wrong" glyph, and an `image://theme/...` id that
is missing on a device fails silently — a security warning that quietly renders nothing is
worse than none at all. Twenty lines of arc and rectangle in `Theme.errorColor` render the
same on every device and cannot go missing.

## Consequences
`Settings` owns `displayAddress` next to `urlForInput`, which is the same translation in
the other direction: one turns what a person typed into a url, the other turns a url into
what a person reads. Both are pure and both are covered by table-driven tests.

The warning is only as good as the engine's own reporting, and this project cannot test
that on a host. If `security` never arrives, tuuli shows no padlock rather than a false
one; that failure is silent, and it is the reason the manual checklist has a line for a
site with a bad certificate.
