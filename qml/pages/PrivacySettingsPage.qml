// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Privacy: how much of the engine's own tracking protection is on, as Firefox for
// Android keeps it under Privacy and security (docs/DECISIONS/0028-settings-pages.md).
// Clearing what browsing leaves behind has a page of its own, History
// (docs/DECISIONS/0030-history-settings.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: privacyPage

    objectName: "privacySettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Privacy")
            }

            // Firefox's tracking protection categories, least first, with Off in place
            // of Custom; the index is the stored value -- PrivacySettings.TrackingProtectionOff,
            // TrackingProtectionStandard, TrackingProtectionStrict
            // (docs/DECISIONS/0023-tracking-protection.md). The description promises
            // only what every engine this runs on does.
            ComboBox {
                objectName: "trackingProtectionCombo"
                width: parent.width
                label: qsTr("Tracking protection")
                description: currentIndex === PrivacySettings.TrackingProtectionOff
                             ? qsTr("Sites can follow you from one to another")
                             : currentIndex === PrivacySettings.TrackingProtectionStrict
                               ? qsTr("Stops more tracking, and can break some sites")
                               : qsTr("Stops sites following you with cookies")
                currentIndex: PrivacySettings.trackingProtection
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Off")
                    }

                    MenuItem {
                        text: qsTr("Standard")
                    }

                    MenuItem {
                        text: qsTr("Strict")
                    }
                }
                onCurrentIndexChanged: PrivacySettings.trackingProtection = currentIndex
            }
        }

        VerticalScrollDecorator {}
    }
}
