// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Settings: a way to each subject -- the start page, search, the reader view, the cover,
// privacy, the history -- on a page of its own, in headed groups, as Firefox for
// Android arranges its settings and Jolla's own browser reaches its privacy settings
// (docs/DECISIONS/0028-settings-pages.md, 0030-history-settings.md). Each way in is its
// icon and its name alone: how a subject is set is read on its page. The one setting
// that takes a line is here too, under the heading of what it changes, and says what it
// does.
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

    function open(page) {
        pageStack.push(Qt.resolvedUrl(page))
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Settings")
            }

            SectionHeader {
                text: qsTr("General")
            }

            // First, as sailfish-browser puts its home page first: the start page is
            // this browser's home page (docs/DECISIONS/0032-start-page.md).
            SettingsEntry {
                objectName: "startPageSettingsEntry"
                // sailfish-browser's for its home page
                // (apps/browser/qml/pages/SettingsPage.qml:75).
                iconSource: "image://theme/icon-m-home"
                text: qsTr("Start page")
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
                onClicked: settingsPage.open("ReaderSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "coverSettingsEntry"
                // The one sailfish-browser's toolbar writes the tab count into
                // (apps/browser/qml/pages/components/ToolBar.qml:162), which is what
                // the cover can be set to show (docs/DECISIONS/0031-cover-is-lightning.md).
                // Not icon-m-display: sailfish-browser has that for its notch guard,
                // which is the screen cutout below.
                iconSource: "image://theme/icon-m-tabs"
                text: qsTr("Cover")
                onClicked: settingsPage.open("CoverSettingsPage.qml")
            }

            // How pages and the grid sit on the screen, so under Appearance, last: a
            // switch rather than a page, and the only line here that says what it does
            // (docs/DECISIONS/0013-screen-cutout.md).
            TextSwitch {
                objectName: "cutoutGuardSwitch"
                text: qsTr("Avoid the screen cutout")
                description: qsTr("Keep pages and the tab grid out from under the camera cutout")
                checked: Settings.cutoutGuard
                onCheckedChanged: Settings.cutoutGuard = checked
            }

            SectionHeader {
                text: qsTr("Privacy")
            }

            SettingsEntry {
                objectName: "privacySettingsEntry"
                // sailfish-browser's for a secure connection
                // (apps/browser/qml/pages/components/CertificateInfo.qml:65).
                iconSource: "image://theme/icon-m-device-lock"
                text: qsTr("Privacy")
                onClicked: settingsPage.open("PrivacySettingsPage.qml")
            }

            SettingsEntry {
                objectName: "historySettingsEntry"
                // The menu's own for the history.
                iconSource: "image://theme/icon-m-history"
                text: qsTr("History")
                onClicked: settingsPage.open("HistorySettingsPage.qml")
            }
        }

        VerticalScrollDecorator {}
    }
}
