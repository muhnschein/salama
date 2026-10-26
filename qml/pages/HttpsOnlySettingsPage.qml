// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// No private tabs, so one switch. Off: engine still tries HTTPS first, falls back to HTTP.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: httpsOnlyPage

    objectName: "httpsOnlySettingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

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
