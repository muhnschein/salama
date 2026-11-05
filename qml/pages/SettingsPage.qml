// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Every row opens page, names no value: no tap here changes anything. Switches, choices one
// level down beside what they belong with.
import QtQuick 2.6
import Sailfish.Silica 1.0
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

        // Help, not setting: pulley, as Silica apps keep it.
        PullDownMenu {
            objectName: "settingsPulley"

            MenuItem {
                objectName: "tutorialMenuItem"
                text: qsTr("Tutorial")
                onClicked: settingsPage.open("TutorialPage.qml")
            }
        }

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Settings")
            }

            SettingsEntry {
                objectName: "searchSettingsEntry"
                iconSource: "image://theme/icon-m-search"
                text: qsTr("Search")
                onClicked: settingsPage.open("SearchSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "startPageSettingsEntry"
                iconSource: "image://theme/icon-m-home"
                text: qsTr("Start page")
                onClicked: settingsPage.open("StartPageSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "appearanceSettingsEntry"
                iconSource: "image://theme/icon-m-ambience"
                text: qsTr("Appearance")
                onClicked: settingsPage.open("AppearanceSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "readerSettingsEntry"
                iconSource: "image://theme/icon-m-file-formatted"
                text: qsTr("Reader view")
                onClicked: settingsPage.open("ReaderSettingsPage.qml")
            }

            SettingsEntry {
                objectName: "coverSettingsEntry"
                iconSource: "image://theme/icon-m-tabs"
                text: qsTr("Cover")
                onClicked: settingsPage.open("CoverSettingsPage.qml")
            }

            // What browser keeps for you, not protection: beside browsing rows.
            SettingsEntry {
                objectName: "historySettingsEntry"
                iconSource: "image://theme/icon-m-history"
                text: qsTr("History")
                onClicked: settingsPage.open("HistorySettingsPage.qml")
            }

            SettingsEntry {
                objectName: "privacySettingsEntry"
                iconSource: "image://theme/icon-m-device-lock"
                text: qsTr("Privacy and security")
                onClicked: settingsPage.open("PrivacySettingsPage.qml")
            }
        }

        VerticalScrollDecorator {}
    }
}
