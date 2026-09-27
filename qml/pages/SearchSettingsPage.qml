// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Search: which engine the address bar searches with, and what on the phone it
// suggests from as something is typed -- the two Firefox for Android keeps on its own
// Search page (docs/DECISIONS/0028-settings-pages.md). The engines are few, so every one
// is on the screen at once, one tap to change: TextSwitches that do not check
// themselves, the one chosen lit.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Page {
    id: searchSettingsPage

    objectName: "searchSettingsPage"
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: qsTr("Search")
            }

            SectionHeader {
                text: qsTr("Search engine")
            }

            Repeater {
                model: SearchSettings.engineNames

                TextSwitch {
                    objectName: "searchEngineChoice"
                    automaticCheck: false
                    text: modelData
                    checked: SearchSettings.engineIndex === index
                    onClicked: SearchSettings.engineIndex = index
                }
            }

            // Where the rows the address bar lists come from, each on until it is
            // switched off here; one switched off lists nothing and counts nothing
            // (docs/DECISIONS/0027-omnibar.md). Firefox calls these "Address bar -
            // Firefox Suggest", which says more about Firefox than about the rows.
            SectionHeader {
                text: qsTr("Address bar suggestions")
            }

            TextSwitch {
                objectName: "omnibarTabsSwitch"
                text: qsTr("Open tabs")
                checked: SearchSettings.omnibarTabs
                onCheckedChanged: SearchSettings.omnibarTabs = checked
            }

            TextSwitch {
                objectName: "omnibarBookmarksSwitch"
                text: qsTr("Bookmarks")
                checked: SearchSettings.omnibarBookmarks
                onCheckedChanged: SearchSettings.omnibarBookmarks = checked
            }

            TextSwitch {
                objectName: "omnibarHistorySwitch"
                text: qsTr("History")
                checked: SearchSettings.omnibarHistory
                onCheckedChanged: SearchSettings.omnibarHistory = checked
            }

            TextSwitch {
                objectName: "omnibarDownloadsSwitch"
                text: qsTr("Downloads")
                checked: SearchSettings.omnibarDownloads
                onCheckedChanged: SearchSettings.omnibarDownloads = checked
            }
        }

        VerticalScrollDecorator {}
    }
}
