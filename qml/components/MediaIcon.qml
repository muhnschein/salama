// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The glyph of a tab's mute, the same on the grid's previews and on the navigation
// bar: the speaker while the tab's sound is on, and the speaker struck through while
// it is muted. Theme ids that are in use elsewhere, none guessed: those of the Harbour
// players that mute (Jupii, harbour-sailfishconnect). There is no small size of
// either, so the medium ones are drawn smaller -- a step up from the small size, the
// one between it and the bar's own controls (docs/DECISIONS/0026-media-controls.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Icon {
    property bool muted: false

    width: Theme.iconSizeSmallPlus
    height: width
    source: muted ? "image://theme/icon-m-speaker-mute" : "image://theme/icon-m-speaker-on"
}
