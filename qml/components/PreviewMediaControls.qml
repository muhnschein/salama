// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What a tab's page plays, over the foot of its preview in the grid: pause while it
// plays and play once paused from here, and beside it the tab's sound, on or muted. As
// Firefox for Android put play and pause on its tab previews, and Firefox and Safari
// put the mute on the tab (docs/DECISIONS/0024-media-controls.md). Nothing is drawn
// for a page that plays nothing, unless its tab is muted.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Row {
    id: controls

    // A TabModel.MediaState, and the tab's muted flag.
    property int mediaState: TabModel.NoMedia
    property bool muted: false

    signal playbackToggled()
    signal muteToggled()

    visible: mediaState !== TabModel.NoMedia || muted

    PreviewButton {
        objectName: "previewPlaybackButton"
        markName: "previewPlaybackMark"
        visible: controls.mediaState !== TabModel.NoMedia
        onClicked: controls.playbackToggled()

        MediaIcon {
            anchors.centerIn: parent
            control: "playback"
            mediaState: controls.mediaState
        }
    }

    PreviewButton {
        objectName: "previewMuteButton"
        markName: "previewMuteMark"
        onClicked: controls.muteToggled()

        MediaIcon {
            anchors.centerIn: parent
            control: "mute"
            muted: controls.muted
        }
    }
}
