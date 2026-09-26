// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Start page: what a tab with no address shows, which is this browser's home page --
// there is no other (docs/DECISIONS/0032-start-page.md). A blank page, or Firefox's
// home: the sites visited most, the bookmarks and the pages read last, each section
// switched on or off. The sections keep their switches while the page is blank, and are
// dimmed.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: startPageSettings

    objectName: "startPageSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Start page")
            }

            ComboBox {
                objectName: "startPageCombo"
                width: parent.width
                label: qsTr("Shows")
                currentIndex: StartPageSettings.blank ? 1 : 0
                menu: ContextMenu {
                    MenuItem {
                        text: qsTr("Your sites")
                    }

                    MenuItem {
                        text: qsTr("A blank page")
                    }
                }
                onCurrentIndexChanged: StartPageSettings.blank = currentIndex === 1
            }

            TextSwitch {
                objectName: "startPageTopSitesSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Frequently visited")
                description: qsTr("Tiles for the sites you visit most")
                checked: StartPageSettings.topSites
                onCheckedChanged: StartPageSettings.topSites = checked
            }

            TextSwitch {
                objectName: "startPageBookmarksSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Bookmarks")
                description: qsTr("Tiles for your first bookmarks")
                checked: StartPageSettings.bookmarks
                onCheckedChanged: StartPageSettings.bookmarks = checked
            }

            TextSwitch {
                objectName: "startPageRecentSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Recently visited")
                description: qsTr("The pages you read last")
                checked: StartPageSettings.recent
                onCheckedChanged: StartPageSettings.recent = checked
            }
        }

        VerticalScrollDecorator {}
    }
}
