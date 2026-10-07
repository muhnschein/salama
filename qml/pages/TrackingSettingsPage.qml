// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Silica has no radio button: TextSwitch with automaticCheck off, checked when level set.
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

            // Stored values least first: Off, Standard, Strict.
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

            // Engine has only part of Firefox protection; no tracker lists on any engine yet.
            Item {
                width: parent.width
                height: Theme.paddingLarge * 2
            }

            Label {
                objectName: "trackingProtectionLimits"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The web engine on Sailfish OS can’t yet do everything Firefox does "
                           + "here, so some trackers may still get through. Salama turns on "
                           + "every protection the engine has.")
            }
        }

        VerticalScrollDecorator {}
    }
}
