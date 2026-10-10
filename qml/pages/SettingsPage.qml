// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Controls write live; no Save.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: settingsPage

    objectName: "settingsPage"
    allowedOrientations: Orientation.Portrait | Orientation.LandscapeMask

    // Notification permissions asked of engine.
    property SettingNames names: SettingNames {}
    // Revision read -> re-query on bookmark change.
    readonly property int coverBookmark: (BookmarkModel.revision,
                                          BookmarkModel.hasBookmark(CoverSettings.quickActionBookmark)
                                          ? CoverSettings.quickActionBookmark
                                          : BookmarkModel.idForUrl(CoverSettings.quickActionBookmarkUrl))
    // Entries flow two a row on landscape.
    readonly property real columnWidth: isLandscape ? width / 2 : width

    function open(page) {
        pageStack.push(Qt.resolvedUrl(page))
    }

    Component.onCompleted: {
        NotificationPermissions.refresh()
        SitePermissions.refresh()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                width: parent.width
                text: qsTr("Browsing")
            }

            Flow {
                width: parent.width

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "startPageSettingsEntry"
                    iconSource: "image://theme/icon-m-home"
                    text: qsTr("Start page")
                    value: settingsPage.names.startPage(StartPageSettings.blank)
                    onClicked: settingsPage.open("StartPageSettingsPage.qml")
                }

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "searchSettingsEntry"
                    iconSource: "image://theme/icon-m-search"
                    text: qsTr("Search")
                    value: SearchEngines.engineNames[SearchSettings.engineIndex] || ""
                    onClicked: settingsPage.open("SearchSettingsPage.qml")
                }
            }

            SectionHeader {
                width: parent.width
                text: qsTr("Appearance")
            }

            Flow {
                width: parent.width

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "readerSettingsEntry"
                    iconSource: "image://theme/icon-m-file-formatted"
                    text: qsTr("Reader view")
                    value: settingsPage.names.reader(ReaderSettings.colors, ReaderSettings.typeface,
                                                     ReaderSettings.textSize)
                    onClicked: settingsPage.open("ReaderSettingsPage.qml")
                }

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "coverSettingsEntry"
                    // Not icon-m-display: that's notch guard's.
                    iconSource: "image://theme/icon-m-tabs"
                    text: qsTr("Cover")
                    value: settingsPage.names.cover(
                               CoverSettings.quickAction, settingsPage.coverBookmark,
                               (BookmarkModel.revision,
                                BookmarkModel.titleOf(settingsPage.coverBookmark)))
                    onClicked: settingsPage.open("CoverSettingsPage.qml")
                }

                // Index = stored value.
                SettingsComboBox {
                    width: settingsPage.columnWidth
                    objectName: "websiteColorsCombo"
                    iconSource: "image://theme/icon-m-night"
                    label: qsTr("Preferred color scheme")
                    description: qsTr("The website style to use when available")
                    currentIndex: Settings.websiteColors
                    menu: ContextMenu {
                        MenuItem {
                            text: settingsPage.names.websiteColors(Settings.WebsiteColorsAutomatic)
                        }

                        MenuItem {
                            text: settingsPage.names.websiteColors(Settings.WebsiteColorsLight)
                        }

                        MenuItem {
                            text: settingsPage.names.websiteColors(Settings.WebsiteColorsDark)
                        }
                    }
                    onCurrentIndexChanged: Settings.websiteColors = currentIndex
                }

                SettingsComboBox {
                    width: settingsPage.columnWidth
                    objectName: "notchGuardCombo"
                    iconSource: "image://theme/icon-m-display"
                    label: qsTr("Notch guard")
                    description: qsTr("Keeps website content away from the screen notch. Automatic "
                                      + "lets adapted websites use the notch area while keeping "
                                      + "content clear.")
                    currentIndex: Settings.notchGuard
                    menu: ContextMenu {
                        MenuItem {
                            text: settingsPage.names.notchGuard(Settings.NotchGuardAutomatic)
                        }

                        MenuItem {
                            text: settingsPage.names.notchGuard(Settings.NotchGuardForced)
                        }

                        MenuItem {
                            text: settingsPage.names.notchGuard(Settings.NotchGuardDisabled)
                        }
                    }
                    onCurrentIndexChanged: Settings.notchGuard = currentIndex
                }

                SettingsSwitch {
                    width: settingsPage.columnWidth
                    objectName: "fixedToolbarSwitch"
                    text: qsTr("Fixed toolbar")
                    description: qsTr("Always show the bottom toolbar")
                    checked: Settings.fixedToolbar
                    onCheckedChanged: Settings.fixedToolbar = checked
                }
            }

            SectionHeader {
                width: parent.width
                text: qsTr("Privacy")
            }

            Flow {
                width: parent.width

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "httpsOnlySettingsEntry"
                    iconSource: "image://theme/icon-m-keys"
                    text: qsTr("HTTPS-Only Mode")
                    value: settingsPage.names.httpsOnly(PrivacySettings.httpsOnly)
                    onClicked: settingsPage.open("HttpsOnlySettingsPage.qml")
                }

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "dohSettingsEntry"
                    iconSource: "image://theme/icon-m-browser"
                    text: qsTr("DNS over HTTPS")
                    value: settingsPage.names.doh(DohSettings.protection)
                    onClicked: settingsPage.open("DohSettingsPage.qml")
                }

                // Named for its page's only item so heading isn't repeated.
                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "trackingSettingsEntry"
                    iconSource: "image://theme/icon-m-device-lock"
                    text: qsTr("Tracking protection")
                    value: settingsPage.names.trackingProtection(PrivacySettings.trackingProtection)
                    onClicked: settingsPage.open("TrackingSettingsPage.qml")
                }

                SettingsSwitch {
                    width: settingsPage.columnWidth
                    objectName: "globalPrivacyControlSwitch"
                    text: qsTr("Tell websites not to share & sell data")
                    description: qsTr("Global Privacy Control (GPC)")
                    checked: PrivacySettings.globalPrivacyControl
                    onCheckedChanged: PrivacySettings.globalPrivacyControl = checked
                }

                SettingsSwitch {
                    width: settingsPage.columnWidth
                    objectName: "javascriptSwitch"
                    text: qsTr("Enable JavaScript")
                    description: checked ? qsTr("Allowed (recommended)")
                                         : qsTr("Blocked, some sites may not work correctly")
                    checked: PrivacySettings.javascript
                    onCheckedChanged: PrivacySettings.javascript = checked
                }

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "sitePermissionsSettingsEntry"
                    iconSource: "image://theme/icon-m-browser-permissions"
                    text: qsTr("Site permissions")
                    value: settingsPage.names.sitePermissions(SitePermissions.exceptionSiteCount)
                    onClicked: settingsPage.open("SitePermissionsPage.qml")
                }

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "historySettingsEntry"
                    iconSource: "image://theme/icon-m-history"
                    text: qsTr("History")
                    value: settingsPage.names.history(PrivacySettings.rememberHistory,
                                                      PrivacySettings.clearHistoryOnClose)
                    onClicked: settingsPage.open("HistorySettingsPage.qml")
                }
            }

            SectionHeader {
                width: parent.width
                text: qsTr("Help")
            }

            Flow {
                width: parent.width

                SettingsEntry {
                    width: settingsPage.columnWidth
                    objectName: "tutorialSettingsEntry"
                    iconSource: "image://theme/icon-m-gesture"
                    text: qsTr("Tutorial")
                    onClicked: settingsPage.open("TutorialPage.qml")
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
