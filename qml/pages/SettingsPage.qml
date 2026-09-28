// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Settings: a way to each subject -- the start page, search, the reader view, the cover,
// tracking protection, notifications, the history -- on a page of its own, in headed
// groups, as Firefox for Android arranges its settings and Jolla's own browser reaches
// its privacy settings (docs/DECISIONS/0028-settings-pages.md, 0030-history-settings.md,
// 0033-web-notifications.md), and last the way to the tutorial (0034-tutorial.md). Each
// way in is its icon, its name, and under the name how the subject is set now, as
// Jolla's Settings writes under each of its own; the page says at a glance how the
// browser is set, and a tap changes it. The two settings that take a line are here too,
// under the heading of what they change: the website colours, a choice of three, and the
// screen cutout's switch.
//
// Every control writes its setting as it changes; nothing waits on a Save.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: settingsPage

    objectName: "settingsPage"
    allowedOrientations: Orientation.Portrait

    // How each subject is set is read as the page comes; the sites' notification
    // permissions are the engine's, and asked of it (docs/DECISIONS/0033-web-notifications.md).
    property SettingNames names: SettingNames {}
    // The cover's quick action's bookmark as it is now, found as the cover's own page
    // finds it (CoverSettingsPage.qml); the revision is read so that it is asked again
    // as the bookmarks change.
    readonly property int coverBookmark: (BookmarkModel.revision,
                                          BookmarkModel.hasBookmark(CoverSettings.quickActionBookmark)
                                          ? CoverSettings.quickActionBookmark
                                          : BookmarkModel.idForUrl(CoverSettings.quickActionBookmarkUrl))

    function open(page) {
        pageStack.push(Qt.resolvedUrl(page))
    }

    Component.onCompleted: NotificationPermissions.refresh()

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            // What browsing starts from and goes by: Firefox for Android's General,
            // named for what is under it.
            SectionHeader {
                text: qsTr("Browsing")
            }

            // First, as sailfish-browser puts its home page first: the start page is
            // this browser's home page (docs/DECISIONS/0032-start-page.md).
            SettingsEntry {
                objectName: "startPageSettingsEntry"
                // sailfish-browser's for its home page
                // (apps/browser/qml/pages/SettingsPage.qml:75).
                iconSource: "image://theme/icon-m-home"
                text: qsTr("Start page")
                value: settingsPage.names.startPage(StartPageSettings.blank)
                onClicked: settingsPage.open("StartPageSettingsPage.qml")
            }

            // What the address bar searches with and suggests from is what a browser is
            // used for most, and Firefox for Android puts it first of its own.
            SettingsEntry {
                objectName: "searchSettingsEntry"
                // sailfish-browser's for its search engine
                // (apps/browser/qml/pages/SettingsPage.qml:117).
                iconSource: "image://theme/icon-m-search"
                text: qsTr("Search")
                value: SearchSettings.engineNames[SearchSettings.engineIndex] || ""
                onClicked: settingsPage.open("SearchSettingsPage.qml")
            }

            SectionHeader {
                text: qsTr("Appearance")
            }

            SettingsEntry {
                objectName: "readerSettingsEntry"
                // The menu's own for the reader view, which is Jolla's Documents' for a
                // text document (docs/DECISIONS/0024-reader-view.md).
                iconSource: "image://theme/icon-m-file-formatted"
                text: qsTr("Reader view")
                value: settingsPage.names.reader(ReaderSettings.colors, ReaderSettings.typeface,
                                                 ReaderSettings.textSize)
                onClicked: settingsPage.open("ReaderSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "coverSettingsEntry"
                // The one sailfish-browser's toolbar writes the tab count into
                // (apps/browser/qml/pages/components/ToolBar.qml:162), which the cover
                // says at rest (docs/DECISIONS/0037-cover-is-where-you-were.md).
                // Not icon-m-display: sailfish-browser has that for its notch guard,
                // which is the screen cutout below.
                iconSource: "image://theme/icon-m-tabs"
                text: qsTr("Cover")
                value: settingsPage.names.cover(CoverSettings.quickAction,
                                                settingsPage.coverBookmark,
                                                (BookmarkModel.revision,
                                                 BookmarkModel.titleOf(settingsPage.coverBookmark)))
                onClicked: settingsPage.open("CoverSettingsPage.qml")
            }

            // How pages are asked to colour themselves: one choice of three, made where it
            // is, as Silica makes one (docs/DECISIONS/0035-website-colours.md). The index
            // is the stored value. The icon is sailfish-browser's for its own colour
            // scheme (apps/browser/qml/pages/SettingsPage.qml).
            SettingsComboBox {
                objectName: "websiteColorsCombo"
                iconSource: "image://theme/icon-m-night"
                label: qsTr("Website colours")
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

            // How pages and the grid sit on the screen, so under Appearance, last: a
            // switch rather than a page, whose name says what it does
            // (docs/DECISIONS/0013-screen-cutout.md). The icon is sailfish-browser's for
            // its notch guard (apps/browser/qml/pages/SettingsPage.qml).
            SettingsSwitch {
                objectName: "cutoutGuardSwitch"
                iconSource: "image://theme/icon-m-display"
                text: qsTr("Avoid the screen cutout")
                checked: Settings.cutoutGuard
                onCheckedChanged: Settings.cutoutGuard = checked
            }

            SectionHeader {
                text: qsTr("Privacy")
            }

            // Named for the one thing on its page, so the heading over it is not said
            // twice.
            SettingsEntry {
                objectName: "trackingSettingsEntry"
                // sailfish-browser's for a secure connection
                // (apps/browser/qml/pages/components/CertificateInfo.qml:65).
                iconSource: "image://theme/icon-m-device-lock"
                text: qsTr("Tracking protection")
                value: settingsPage.names.trackingProtection(PrivacySettings.trackingProtection)
                onClicked: settingsPage.open("TrackingSettingsPage.qml")
            }

            // Firefox keeps the sites allowed to send notifications with the other
            // permissions, under Privacy (docs/DECISIONS/0033-web-notifications.md).
            SettingsEntry {
                objectName: "notificationSettingsEntry"
                // The platform's own for notifications.
                iconSource: "image://theme/icon-m-notifications"
                text: qsTr("Notifications")
                value: settingsPage.names.notifications(NotificationPermissions.allowedCount,
                                                        NotificationPermissions.blockedCount,
                                                        PrivacySettings.blockNotificationRequests)
                onClicked: settingsPage.open("NotificationSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "historySettingsEntry"
                // The menu's own for the history.
                iconSource: "image://theme/icon-m-history"
                text: qsTr("History")
                value: settingsPage.names.history(PrivacySettings.rememberHistory,
                                                  PrivacySettings.clearHistoryOnClose)
                onClicked: settingsPage.open("HistorySettingsPage.qml")
            }

            // Last, as Firefox for Android ends its settings with its help: the
            // tutorial the first start shows, again (docs/DECISIONS/0034-tutorial.md).
            SectionHeader {
                text: qsTr("Help")
            }

            SettingsEntry {
                objectName: "tutorialSettingsEntry"
                // The theme's own for a gesture, which is what the tutorial teaches.
                // Jolla's Settings has no row for its Tutorial to take one from.
                iconSource: "image://theme/icon-m-gesture"
                text: qsTr("Tutorial")
                onClicked: settingsPage.open("TutorialPage.qml")
            }
        }

        VerticalScrollDecorator {}
    }
}
