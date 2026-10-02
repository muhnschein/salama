// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// HTTPS-Only Mode: Firefox for Android's page for it, its switch and its words, under
// Privacy (docs/DECISIONS/0047-secure-connections.md). Firefox for Android asks whether
// it is on in all tabs or only in private ones; this browser has no private tabs
// (0019-no-private-tabs.md), so the switch is all there is. While it is off, the engine
// still tries HTTPS first and falls back to HTTP, as Firefox does, and the line under
// the switch says so in desktop Firefox's words.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: httpsOnlyPage

    objectName: "httpsOnlySettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("HTTPS-Only Mode")
            }

            TextSwitch {
                objectName: "httpsOnlySwitch"
                text: qsTr("HTTPS-Only Mode")
                description: qsTr("Automatically attempts to connect to sites using HTTPS "
                                  + "encryption protocol for increased security.")
                checked: PrivacySettings.httpsOnly
                onCheckedChanged: PrivacySettings.httpsOnly = checked
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            Label {
                objectName: "httpsFirstNote"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !PrivacySettings.httpsOnly
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                //: Under the HTTPS-Only Mode switch while it is off: the engine still tries
                //: HTTPS before HTTP. Salama is the browser's name.
                text: qsTr("Salama may still upgrade some connections")
            }
        }

        VerticalScrollDecorator {}
    }
}
