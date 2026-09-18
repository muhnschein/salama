# 0019 — No private tabs

## Context
Private tabs (0017) were meant to end up behind a lock: a private group that opens
only after the device lock code or a fingerprint. The device spike for that lock,
run on Sailfish OS 5.2.0.17 with a code and a fingerprint set, settled it. The only
Harbour-legal way to ask the system for a verification is `Sailfish.Secrets` with a
device-lock collection, and what the system answers with is a confirmation dialog —
"allow the application to read a secret?" — never the code and never the finger. The
daemon's own fallback path asks for confirmation alone by design, and the polkit path
the launcher takes ends at lipstick's security UI with the same. `Nemo.DeviceLock`,
which asks properly, is not on Harbour's import list, and its D-Bus service is not on
any Sailjail permission a Harbour application may have.

What remained was a PIN of the application's own, with no fingerprint. The person
testing the build declined it, and with it private tabs as such: a private group
without a lock is a group with a "Private" label, and the label would say what the
group could not keep.

## Decision
There are **no private tabs**. `Tab` and `TabGroup` carry no private flag, `newTab`
takes no such argument, the menu offers no "New private tab", the engine's view is
never put in `privateMode`, and every tab is written to history, offered a preview
and recorded when closed. The strip starts with the default group alone.

Storage is **schema 6**: the `private` column is gone from `tab` and `tab_group`.
SQLite before 3.35 cannot drop a column, so a table that still has it is rebuilt —
its flagged rows deleted first, then a fresh table created from the schema, the rows
copied across by name, the old table dropped and the fresh one renamed. The ids are
kept, so tabs still name their groups; the group table is rebuilt the same way.
Asking the table for the column rather than the version number keeps the migration
correct whichever schema the file came from, as the earlier ones are.

The engine's "clear private data" message (0006) is unrelated and stays: it is the
name the engine gives clearing cookies and cache from Settings.

## Consequences
0017 is superseded by this record and kept for the history of the idea. Should a
platform way to verify the device lock become available to Harbour applications, the
gate would go where 0017 said — `TabGroups.activate()`, the search model, and an
overlay on resume — and the group and its flag would come back with schema 7.

Fewer strings, one fewer role on three models, and a shorter device checklist.
