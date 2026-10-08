// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tab mute glyph: struck through unless sound actually plays (muted, paused, or behind
// front tab). No small speaker icon exists; medium drawn smaller.
import QtQuick 2.6
import Sailfish.Silica 1.0

Icon {
    property bool heard: false

    width: Theme.iconSizeSmallPlus
    height: width
    source: heard ? "image://theme/icon-m-speaker-on" : "image://theme/icon-m-speaker-mute"
}
