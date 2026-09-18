# 0016 — Five pages stay loaded; the rest reload when they come back

## Context
0003 gave every tab shown in a session its own `WebView` and never let one go, and
said that a cap would come if the smoke test showed pressure. The research behind
0015 looked at what the platform does about this and found the answer written in
Jolla's own browser: `WebPageQueue` in `sailfish-browser` keeps **five** pages live,
most recently used first, deletes the rest and reloads a page from its URL when its
tab comes back; on the `com.nokia.mce` memory signal it tells the engine
`"memory-pressure"` / `"low-memory"` and drops every page but the active one; and after
600 seconds in the background it sends `"heap-minimize"`. Sailfish OS itself dictates
nothing per application: there is no cap, only the kernel's out-of-memory killer, which
takes the whole process.

## Decision
`TabModel` carries a `liveTab` role: true for the tab in front and the tabs most
recently in front before it, up to `Settings.liveTabLimit`, which is **5 by default** —
the platform browser's number — and 3, 10 or all in Settings ("Pages kept loaded").
The order is the activation stamp the cover already uses (0014), so nothing new is
counted. The browsing page's `Loader` is active only for a tab that is both shown and
live: a tab beyond the limit gives its view up, and when it comes to the front again
the `Loader` builds a new one from `model.url`, the page it was on, through the same
path a restored tab takes. What is lost is what Jolla's browser loses: the engine's
back and forward history for that tab, its scroll position and its form state. The
grid's preview stays, because it is a file (0008).

After **ten minutes** in the background the page tells the engine
`"memory-pressure"` / `"heap-minimize"`, with the words in `EngineMessages` beside the
other engine strings (0006) and through `WebEngine.notifyObservers`, which Settings
already uses to clear data. It asks the engine to give back what it can; it evicts
nothing, because coming back to a browser and finding every page reloading is worse
than the memory.

The `mce` memory signal is **not** listened to yet. `Nemo.DBus 2.0` is on Harbour's
list and could subscribe to it, but whether Sailjail lets a Harbour application hear
that signal is a device question, and the research record says so. The two functions
above are what can be verified without a phone.

## Consequences
A tab that has fallen out of the five comes back a moment later than it left, from the
top of its page. Settings says as much under the choice. Restored tabs are unchanged:
they cost nothing until tapped, and a restored tab never in front this session has an
old stamp, so it does not take a slot from a page that is actually loaded.

The load tests set the limit to three through the Settings combo and count `WebView`s
as tabs are opened and brought back; the ten-minute trim is a named function the tests
call, with the engine stub recording what it was told. What the engine does with
`heap-minimize`, and how much a dropped view actually returns, are on the device
checklist.
