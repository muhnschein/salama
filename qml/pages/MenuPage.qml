// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

Page {
    id: menuPage

    // The page this menu was opened from, so the tab grid can be reached through it:
    // the grid is part of that page now, not a page of its own.
    property Item browserPage

    objectName: "menuPage"
    allowedOrientations: Orientation.Portrait

    ShareAction {
        id: shareAction

        objectName: "shareAction"
        mimeType: "text/x-url"
        resources: [{
                "type": "text/x-url",
                "linkTitle": TabModel.activeTitle,
                "status": TabModel.activeUrl
            }]
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: TabModel.activeTitle.length > 0 ? TabModel.activeTitle : qsTr("Menu")
                description: TabModel.activeUrl
            }

            ListItem {
                objectName: "newTabItem"
                onClicked: {
                    TabModel.newTab(Settings.homePage)
                    pageStack.pop()
                }

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("New tab")
                }
            }

            ListItem {
                objectName: "bookmarkItem"
                enabled: TabModel.activeUrl.length > 0
                onClicked: {
                    if (BookmarkModel.activeUrlBookmarked) {
                        BookmarkModel.removeByUrl(TabModel.activeUrl)
                    } else {
                        BookmarkModel.add(TabModel.activeUrl, TabModel.activeTitle,
                                          TabModel.activeFavicon)
                    }
                    pageStack.pop()
                }

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: BookmarkModel.activeUrlBookmarked ? qsTr("Remove bookmark")
                                                            : qsTr("Bookmark this page")
                }
            }

            ListItem {
                objectName: "shareItem"
                enabled: TabModel.activeUrl.length > 0
                onClicked: shareAction.trigger()

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Share")
                }
            }

            SectionHeader {
                text: qsTr("Browse")
            }

            // The tab grid is opened by dragging the navigation bar upwards. This is
            // the way there that does not need the gesture -- on a device where the
            // drag is awkward, or a hand that is already in the menu.
            ListItem {
                objectName: "tabsItem"
                onClicked: {
                    pageStack.pop()
                    if (menuPage.browserPage) {
                        menuPage.browserPage.showTabs()
                    }
                }

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Tabs")
                }
            }

            // The tab in front, into another group: the one way a tab changes group,
            // here beside the grid it leaves rather than on a cell that already has a
            // tap, a carry and a close button on it.
            ListItem {
                objectName: "moveToGroupItem"
                enabled: TabModel.activeTabId > 0
                onClicked: pageStack.replace(Qt.resolvedUrl("TabGroupsPage.qml"), {
                                                 "moveTabId": TabModel.activeTabId
                                             })

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Move tab to group")
                }
            }

            ListItem {
                objectName: "bookmarksItem"
                onClicked: pageStack.replace(Qt.resolvedUrl("BookmarksPage.qml"))

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Bookmarks")
                }
            }

            ListItem {
                objectName: "historyItem"
                onClicked: pageStack.replace(Qt.resolvedUrl("HistoryPage.qml"))

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("History")
                }
            }

            ListItem {
                objectName: "settingsItem"
                onClicked: pageStack.replace(Qt.resolvedUrl("SettingsPage.qml"))

                Label {
                    x: Theme.horizontalPageMargin
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Settings")
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
