// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Search: which engine the address bar searches with, and what on the phone it
// suggests from as something is typed -- the two Firefox for Android keeps on its own
// Search page (docs/DECISIONS/0028-settings-pages.md).
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
            spacing: Theme.paddingMedium

            PageHeader {
                title: qsTr("Search")
            }

            ComboBox {
                objectName: "searchEngineCombo"
                width: parent.width
                label: qsTr("Search engine")
                currentIndex: Settings.searchEngineIndex
                menu: ContextMenu {
                    Repeater {
                        model: Settings.searchEngineNames

                        MenuItem {
                            text: modelData
                        }
                    }
                }
                onCurrentIndexChanged: Settings.searchEngineIndex = currentIndex
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
                checked: Settings.omnibarTabs
                onCheckedChanged: Settings.omnibarTabs = checked
            }

            TextSwitch {
                objectName: "omnibarBookmarksSwitch"
                text: qsTr("Bookmarks")
                checked: Settings.omnibarBookmarks
                onCheckedChanged: Settings.omnibarBookmarks = checked
            }

            TextSwitch {
                objectName: "omnibarHistorySwitch"
                text: qsTr("History")
                checked: Settings.omnibarHistory
                onCheckedChanged: Settings.omnibarHistory = checked
            }

            TextSwitch {
                objectName: "omnibarDownloadsSwitch"
                text: qsTr("Downloads")
                checked: Settings.omnibarDownloads
                onCheckedChanged: Settings.omnibarDownloads = checked
            }
        }

        VerticalScrollDecorator {}
    }
}
