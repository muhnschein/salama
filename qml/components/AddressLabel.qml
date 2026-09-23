// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the navigation bar shows while the address is not being edited: the host, a
// warning beside it when the engine is unhappy with the connection
// (docs/DECISIONS/0011-address-and-security.md), and left of both the media controls
// while the page plays something (docs/DECISIONS/0023-media-controls.md). Safari puts
// its mute in the address field the same way.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Row {
    id: address

    property string url
    property bool tlsBroken: false
    property bool pressed: false
    // What the page plays, a TabModel.MediaState, and whether its tab is muted.
    property int mediaState: TabModel.NoMedia
    property bool muted: false
    // Which of the media controls a finger is on: "playback", "mute" or neither.
    property string pressedControl
    // How far the media controls reach into the row, in its own coordinates: a press
    // left of playbackEnd is the first one's, and up to mediaEnd the second one's. Half
    // the spacing either side of each is its own, so the two tile what they take.
    readonly property bool showsPlayback: playbackIcon.visible
    readonly property bool showsMute: muteIcon.visible
    readonly property real playbackEnd: playbackIcon.x + playbackIcon.width + spacing / 2
    readonly property real mediaEnd: muteIcon.x + muteIcon.width + spacing / 2
    // The widest this may be drawn. The label takes what the warning leaves of it.
    property real maximumWidth: 0
    property real fontSize: Theme.fontSizeMedium

    spacing: Theme.paddingSmall

    MediaIcon {
        id: playbackIcon

        objectName: "playbackButton"
        anchors.verticalCenter: parent.verticalCenter
        visible: address.mediaState !== TabModel.NoMedia
        control: "playback"
        mediaState: address.mediaState
        highlighted: address.pressedControl === "playback"
    }

    MediaIcon {
        id: muteIcon

        objectName: "muteButton"
        anchors.verticalCenter: parent.verticalCenter
        visible: address.mediaState !== TabModel.NoMedia || address.muted
        control: "mute"
        muted: address.muted
        highlighted: address.pressedControl === "mute"
    }

    // The platform's own warning glyph in the error colour: there is no open padlock
    // in the icon set, and sailfish-browser draws this one for this state.
    Icon {
        id: securityIcon

        objectName: "securityWarning"
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.iconSizeSmall
        height: width
        visible: address.tlsBroken
        source: "image://theme/icon-s-filled-warning"
        color: Theme.errorColor
    }

    Label {
        objectName: "addressLabel"
        anchors.verticalCenter: parent.verticalCenter
        // Wide enough for the text and no wider, so the row centres on what is drawn
        // rather than on the room it was given.
        width: Math.min(implicitWidth, address.maximumWidth
                        - (securityIcon.visible ? securityIcon.width + address.spacing : 0)
                        - (playbackIcon.visible ? playbackIcon.width + address.spacing : 0)
                        - (muteIcon.visible ? muteIcon.width + address.spacing : 0))
        // The host, not the whole url (Settings.displayAddress). Tapping the bar
        // brings the field up with every character of it back.
        text: address.url.length > 0 ? Settings.displayAddress(address.url)
                                     : qsTr("Search or enter address")
        truncationMode: TruncationMode.Fade
        color: address.pressed ? Theme.highlightColor : Theme.primaryColor
        font.pixelSize: address.fontSize
    }
}
