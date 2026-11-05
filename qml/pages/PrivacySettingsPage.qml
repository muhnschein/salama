// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// All that protects you, one page (Safari's Privacy & Security). Firefox's tracking protection
// levels and words; GPC under them, as on Firefox's page. Silica has no radio button: TextSwitch
// with automaticCheck off, checked when level set.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: privacyPage

    property SettingNames names: SettingNames {}

    objectName: "privacySettingsPage"
    allowedOrientations: Orientation.Portrait

    BrowsingDataClearer {
        id: clearer

        page: privacyPage
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Privacy and security")
            }

            SectionHeader {
                text: qsTr("Tracking protection")
            }

            // Stored values least first: Off, Standard, Strict.
            Repeater {
                model: [PrivacySettings.TrackingProtectionOff,
                        PrivacySettings.TrackingProtectionStandard,
                        PrivacySettings.TrackingProtectionStrict]

                TextSwitch {
                    objectName: "trackingProtectionChoice"
                    automaticCheck: false
                    text: privacyPage.names.trackingProtection(modelData)
                    description: privacyPage.names.trackingProtectionDescription(modelData)
                    checked: PrivacySettings.trackingProtection === modelData
                    onClicked: PrivacySettings.trackingProtection = modelData
                }
            }

            // Engine has only part of Firefox protection; no tracker lists on any engine yet.
            Label {
                objectName: "trackingProtectionLimits"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                topPadding: Theme.paddingSmall
                bottomPadding: Theme.paddingLarge
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The web engine on Sailfish OS can’t yet do everything Firefox does "
                           + "here, so some trackers may still get through. Salama turns on "
                           + "every protection the engine has.")
            }

            // Line apart: GPC its own switch, not fourth level.
            Separator {
                objectName: "globalPrivacyControlSeparator"
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                color: Theme.primaryColor
                horizontalAlignment: Qt.AlignHCenter
            }

            TextSwitch {
                objectName: "globalPrivacyControlSwitch"
                text: qsTr("Tell websites not to share & sell data")
                description: qsTr("Global Privacy Control (GPC)")
                checked: PrivacySettings.globalPrivacyControl
                onCheckedChanged: PrivacySettings.globalPrivacyControl = checked
            }

            SectionHeader {
                //: HTTPS-Only Mode and DNS over HTTPS: how the browser connects to sites
                text: qsTr("Secure connections")
            }

            // HTTPS-First always on (EngineMessages::httpsOnlyPreferences): switch adds only
            // refusal of plain HTTP. Description says which, for state shown.
            TextSwitch {
                objectName: "httpsOnlySwitch"
                text: qsTr("HTTPS-Only Mode")
                description: checked
                             //: Under HTTPS-Only Mode while on
                             ? qsTr("HTTPS only. Asks before opening a site without HTTPS")
                             //: Under HTTPS-Only Mode while off
                             : qsTr("Tries HTTPS first, uses HTTP if a site has no HTTPS")
                checked: PrivacySettings.httpsOnly
                onCheckedChanged: PrivacySettings.httpsOnly = checked
            }

            SettingsEntry {
                objectName: "dohSettingsEntry"
                iconSource: "image://theme/icon-m-browser"
                text: qsTr("DNS over HTTPS")
                onClicked: pageStack.push(Qt.resolvedUrl("DohSettingsPage.qml"))
            }

            SectionHeader {
                //: What sites may do, and what they have left on the phone
                text: qsTr("Sites")
            }

            SettingsEntry {
                objectName: "sitePermissionsSettingsEntry"
                iconSource: "image://theme/icon-m-browser-permissions"
                text: qsTr("Site permissions")
                onClicked: pageStack.push(Qt.resolvedUrl("SitePermissionsPage.qml"))
            }

            // Row, not button: Firefox's and sailfish-browser's place for it.
            SettingsEntry {
                objectName: "clearDataSettingsEntry"
                iconSource: "image://theme/icon-m-delete"
                text: qsTr("Clear browsing data")
                onClicked: clearer.open()
            }
        }

        VerticalScrollDecorator {}
    }
}
