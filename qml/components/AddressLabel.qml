// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Mute hangs off centred row, not in it: else host leaves centre.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: address

    property string url
    property bool tlsBroken: false
    property bool pressed: false
    property int mediaState: TabModel.NoMedia
    property bool muted: false
    property bool mutePressed: false
    // Press left of muteEnd (row coords) is mute's.
    readonly property bool showsMute: muteIcon.visible
    readonly property real muteEnd: muteIcon.x + muteIcon.width + muteGap / 2
    // Mute counted both sides so row stays centred.
    property real maximumWidth: 0
    property real fontSize: Theme.fontSizeMedium
    property real iconSize: Theme.iconSizeSmallPlus
    readonly property real spacing: Theme.paddingSmall
    readonly property real muteGap: Theme.paddingMedium

    width: row.width
    height: row.height

    MediaIcon {
        id: muteIcon

        objectName: "muteButton"
        anchors {
            right: row.left
            rightMargin: address.muteGap
            verticalCenter: parent.verticalCenter
        }
        width: address.iconSize
        visible: address.mediaState !== TabModel.NoMedia || address.muted
        heard: address.mediaState === TabModel.MediaPlaying && !address.muted
        color: Theme.highlightColor
        highlightColor: Theme.secondaryHighlightColor
        highlighted: address.mutePressed
    }

    Row {
        id: row

        spacing: address.spacing

        // No open padlock in icon set.
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
            // Text width so row centres on drawn text.
            width: Math.min(implicitWidth, address.maximumWidth
                            - (securityIcon.visible ? securityIcon.width + address.spacing : 0)
                            - (muteIcon.visible ? 2 * (muteIcon.width + address.muteGap) : 0))
            text: address.url.length > 0 ? SearchSettings.displayAddress(address.url)
                                         : qsTr("Search or enter address")
            truncationMode: TruncationMode.Fade
            color: address.pressed ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: address.fontSize
        }
    }
}
