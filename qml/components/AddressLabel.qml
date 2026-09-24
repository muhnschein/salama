// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the navigation bar shows while the address is not being edited: the host, a
// warning beside it when the engine is unhappy with the connection
// (docs/DECISIONS/0011-address-and-security.md), and left of both the tab's mute while
// the page plays something (docs/DECISIONS/0026-media-controls.md). Safari puts its
// mute in the address field the same way.
//
// The host and the warning are the row the bar centres. The mute hangs off its left
// rather than being part of it: counted in, it pushed the host off the middle of the
// bar whenever something played.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: address

    property string url
    property bool tlsBroken: false
    property bool pressed: false
    // What the page plays, a TabModel.MediaState, and whether its tab is muted.
    property int mediaState: TabModel.NoMedia
    property bool muted: false
    // A finger is on the mute.
    property bool mutePressed: false
    // How far the mute reaches, in this row's coordinates, which it hangs left of: a
    // press left of muteEnd is the mute's. Half the gap between it and the row is its
    // own.
    readonly property bool showsMute: muteIcon.visible
    readonly property real muteEnd: muteIcon.x + muteIcon.width + muteGap / 2
    // The widest this may be drawn. The label takes what the warning leaves of it, and
    // while the mute is there, what the mute takes on both sides: the row stays in the
    // middle, and the mute must fit beside it.
    property real maximumWidth: 0
    property real fontSize: Theme.fontSizeMedium
    property real iconSize: Theme.iconSizeSmallPlus
    readonly property real spacing: Theme.paddingSmall
    readonly property real muteGap: Theme.paddingMedium

    width: row.width
    height: row.height

    // In the ambience's colour, where the bar's other controls are in the primary one:
    // it says something about the page as much as it is a control.
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
        muted: address.muted
        color: Theme.highlightColor
        highlightColor: Theme.secondaryHighlightColor
        highlighted: address.mutePressed
    }

    Row {
        id: row

        spacing: address.spacing

        // The platform's own warning glyph in the error colour: there is no open
        // padlock in the icon set, and sailfish-browser draws this one for this state.
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
            // Wide enough for the text and no wider, so the row centres on what is
            // drawn rather than on the room it was given.
            width: Math.min(implicitWidth, address.maximumWidth
                            - (securityIcon.visible ? securityIcon.width + address.spacing : 0)
                            - (muteIcon.visible ? 2 * (muteIcon.width + address.muteGap) : 0))
            // The host, not the whole url (Settings.displayAddress). Tapping the bar
            // brings the field up with every character of it back.
            text: address.url.length > 0 ? Settings.displayAddress(address.url)
                                         : qsTr("Search or enter address")
            truncationMode: TruncationMode.Fade
            color: address.pressed ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: address.fontSize
        }
    }
}
