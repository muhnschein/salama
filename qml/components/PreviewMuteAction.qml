// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Own MouseArea above cell handler, so tap never reaches cell.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: action

    property int mediaState: TabModel.NoMedia
    property bool muted: false
    readonly property bool heard: mediaState === TabModel.MediaPlaying && !muted

    // MouseArea clicked carries mouse event tests can't supply.
    signal toggled()

    visible: mediaState !== TabModel.NoMedia || muted
    // Square, no wider than needed: rest of foot still opens tab.
    width: height
    height: Theme.itemSizeSmall

    MouseArea {
        id: tap

        anchors.fill: parent
        onClicked: action.toggled()
    }

    MediaIcon {
        objectName: "previewMuteIcon"
        anchors.centerIn: parent
        heard: action.heard
        highlighted: tap.pressed
    }
}
