# 0017 — One private group, whose tabs are kept

## Context
Private tabs were a flag on a tab: the engine kept their cookies apart, the model kept
them out of history and wrote no preview, and they were never written to the
database, so a restart lost them. With tab groups (0015) the question was where a
private tab lives, and the research that followed proposed what Safari does: a private
group, at one end of the strip, that every private tab belongs to. The person testing the
build asked for one thing more — that private tabs survive a restart like the others.

## Decision
There is always exactly **one private group** (`TabGroup::isPrivate`). It is first in
the strip and stays first, to the left of the default group, where the person testing
asked for it; a new group goes last, and the order is sorted on load so that a database
from the build that kept it last comes up right. It is named "Private" wherever groups
are named and cannot be renamed or deleted; nor can the default group after it (0015),
so the grid always has somewhere to be.

A tab is private because of the group it is in, and only then. `newTab(url, true)` —
the menu's "New private tab" — opens the tab in the private group and makes that group
current; a tab opened while the private group is current is private too. A tab never
moves into or out of the private group: `moveTabToGroup` refuses, and the picker page
does not offer the row, because `privateMode` is a property of the engine's view and
not something a running view can be given or have taken away. On load a private tab
filed under an ordinary group, which only a hand-edited database produces, is moved to
the private group rather than shown among ordinary tabs.

Private tabs are **written to the database with their flag** (schema 5,
`tab.private`), so the private group comes back after a restart with what was in it.
Everything else private stays as it was: no history, no bookmark favicon updates, no
preview on disk, and the engine's private mode for cookies and site data. What is
persisted is the list of addresses, which is the trade-off asked for: a private tab
that vanishes on restart was the one thing the person testing did not want.

The recently-closed list (0018) never records a private tab: closing one is meant to
leave nothing behind, and a tab opened again from that list comes back ordinary, in an
ordinary group.

## Consequences
The grid's placeholder still says "Private tab" for a private cell, since no preview
is ever written. The cover counts private tabs with the rest, and draws their cells as
ground alone, as before.

No lock. The research record named `Sailfish.Secrets` with a device-lock collection as
the one platform-sanctioned way for a Harbour application to re-verify the device lock,
and said to spike it on the phone before building on it; that spike has not happened,
so the private group opens to a tap like any other. When it does, the gate is
`TabGroups.activate()` and the search model, and the record for it goes here.
