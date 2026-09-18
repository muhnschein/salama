// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the navigation bar shows while the address is not being edited: the host, and
// a warning beside it when the engine is unhappy with the connection
// (docs/DECISIONS/0011-address-and-security.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Row {
    id: address

    property string url
    property bool tlsBroken: false
    property bool privateTab: false
    property bool pressed: false
    // The widest this may be drawn. The label takes what the warning leaves of it.
    property real maximumWidth: 0
    property real fontSize: Theme.fontSizeMedium

    spacing: Theme.paddingSmall

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
                        - (securityIcon.visible ? securityIcon.width + address.spacing : 0))
        // The host, not the whole url (Settings.displayAddress). Tapping the bar
        // brings the field up with every character of it back.
        text: address.url.length > 0 ? Settings.displayAddress(address.url)
                                     : qsTr("Search or enter address")
        truncationMode: TruncationMode.Fade
        color: {
            if (address.pressed) {
                return Theme.highlightColor
            }
            return address.privateTab ? Theme.highlightColor : Theme.primaryColor
        }
        font.pixelSize: address.fontSize
    }
}
