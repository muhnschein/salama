// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The glyph of a tab's mute, the same on the grid's previews and on the navigation
// bar: the speaker while the tab plays and is heard, and the speaker struck through
// while it is not -- muted, paused, or held behind the tab in front. What it shows is
// whether a sound comes from the tab, not only whether it was muted: a tab paused as it
// was left showed an unmuted speaker over nothing playing
// (docs/DECISIONS/0026-media-controls.md). Theme ids that are in use elsewhere, none
// guessed: those of the Harbour players that mute (Jupii, harbour-sailfishconnect).
// There is no small size of either, so the medium ones are drawn smaller -- a step up
// from the small size, the one between it and the bar's own controls.
import QtQuick 2.6
import Sailfish.Silica 1.0

Icon {
    property bool heard: false

    width: Theme.iconSizeSmallPlus
    height: width
    source: heard ? "image://theme/icon-m-speaker-on" : "image://theme/icon-m-speaker-mute"
}
