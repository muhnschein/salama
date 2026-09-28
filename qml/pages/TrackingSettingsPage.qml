// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tracking protection: how much of the engine's own is on, as Firefox for Android keeps
// it under Privacy and security (docs/DECISIONS/0023-tracking-protection.md,
// 0028-settings-pages.md). Firefox's categories, least first, with Off in place of
// Custom, each a row saying what it does: all three on the screen at once, one tap to
// change, the one chosen lit. Silica has no radio button: each is a TextSwitch that does
// not check itself (automaticCheck off), checked while its level is the one set, so its
// light is the choice's. Clearing what browsing leaves behind has a page of its own,
// History (docs/DECISIONS/0030-history-settings.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: trackingPage

    property SettingNames names: SettingNames {}

    objectName: "trackingSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Tracking protection")
            }

            // The stored values, least first: PrivacySettings.TrackingProtectionOff,
            // TrackingProtectionStandard, TrackingProtectionStrict.
            Repeater {
                model: [PrivacySettings.TrackingProtectionOff,
                        PrivacySettings.TrackingProtectionStandard,
                        PrivacySettings.TrackingProtectionStrict]

                TextSwitch {
                    objectName: "trackingProtectionChoice"
                    automaticCheck: false
                    text: trackingPage.names.trackingProtection(modelData)
                    description: trackingPage.names.trackingProtectionDescription(modelData)
                    checked: PrivacySettings.trackingProtection === modelData
                    onClicked: PrivacySettings.trackingProtection = modelData
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
