// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Start page: what a tab with no address shows, which is this browser's home page --
// there is no other (docs/DECISIONS/0032-start-page.md). The start page's sections or a
// blank page, both on the screen at once and one tap to change, as TextSwitches that do
// not check themselves, the one chosen lit; then Firefox's home sections, the sites
// visited most, the bookmarks and the pages read last, each switched on or off. The
// sections keep their switches while the page is blank, and are dimmed. Under them, a
// picture of the screen as a new tab will show it follows each switch as it moves.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: startPageSettings

    property SettingNames names: SettingNames {}

    objectName: "startPageSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + 2 * Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Start page")
            }

            TextSwitch {
                objectName: "startPageSitesSwitch"
                automaticCheck: false
                text: startPageSettings.names.startPage(false)
                checked: !StartPageSettings.blank
                onClicked: StartPageSettings.blank = false
            }

            TextSwitch {
                objectName: "startPageBlankSwitch"
                automaticCheck: false
                text: startPageSettings.names.startPage(true)
                checked: StartPageSettings.blank
                onClicked: StartPageSettings.blank = true
            }

            SectionHeader {
                //: The parts of the start page, each switched on or off
                text: qsTr("Sections")
            }

            TextSwitch {
                objectName: "startPageTopSitesSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Frequently visited")
                checked: StartPageSettings.topSites
                onCheckedChanged: StartPageSettings.topSites = checked
            }

            TextSwitch {
                objectName: "startPageBookmarksSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Bookmarks")
                checked: StartPageSettings.bookmarks
                onCheckedChanged: StartPageSettings.bookmarks = checked
            }

            TextSwitch {
                objectName: "startPageRecentSwitch"
                enabled: !StartPageSettings.blank
                text: qsTr("Recently visited")
                checked: StartPageSettings.recent
                onCheckedChanged: StartPageSettings.recent = checked
            }

            SectionHeader {
                //: Over a picture of what a new tab will show
                text: qsTr("Preview")
            }

            Item {
                width: parent.width
                height: Theme.paddingLarge
            }

            StartPagePreview {
                width: parent.width
                screenWidth: startPageSettings.width
                screenHeight: startPageSettings.height
            }
        }

        VerticalScrollDecorator {}
    }
}
