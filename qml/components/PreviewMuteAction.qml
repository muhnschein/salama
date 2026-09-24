// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A tab's mute over the foot of its preview in the grid, drawn as a cover draws its
// quick actions: the glyph alone, centred along the bottom edge, on the ground the
// picture fades out to there rather than on a disc of its own (TabPreview.qml; piirit's
// and vuo's covers make room for what they draw at their foot the same way). The
// speaker while the tab's sound is on, struck through while it is muted, and muting
// pauses it as well (docs/DECISIONS/0024-media-controls.md). Nothing is drawn for a
// page that plays nothing, unless its tab is muted.
//
// It takes its own presses, above the handler the cell's gestures go through, so a
// tap on it is never a tap on the cell.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: action

    // A TabModel.MediaState, and the tab's muted flag.
    property int mediaState: TabModel.NoMedia
    property bool muted: false

    // Its own tap signal: the handler's carries a mouse event, which a test cannot give
    // it.
    signal toggled()

    visible: mediaState !== TabModel.NoMedia || muted
    // As tall as the strip a cover's actions sit in, and the touch target is that
    // square: larger than the glyph, and no wider than it needs to be, so the rest of
    // the foot still opens the tab.
    width: height
    height: Theme.itemSizeSmall

    MouseArea {
        id: tap

        anchors.fill: parent
        onClicked: action.toggled()
    }

    // Lit under a finger, as a cover's actions are.
    MediaIcon {
        objectName: "previewMuteIcon"
        anchors.centerIn: parent
        muted: action.muted
        highlighted: tap.pressed
    }
}
