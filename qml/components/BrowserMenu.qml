// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the menu button on the navigation bar brings up: a sheet of icons that comes
// up from under the bar, each with what it does written under it, in three rows -- the
// tabs, the page in front, the browser. A tap on one does it and puts the sheet away;
// a tap outside it puts it away alone. The same DockedPanel the grid's list of closed
// tabs is, so the two sheets come and go alike (docs/DECISIONS/0021-menu-sheet.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Share 1.0
import harbour.salama 1.0

DockedPanel {
    id: menu

    // The page in front, which the second row acts on, or null while it is made.
    property Item view: null
    readonly property bool hasPage: TabModel.activeUrl.length > 0

    // Searching the page is done over the navigation bar, which the browsing page has.
    signal findRequested()

    objectName: "browserMenu"
    width: parent.width
    height: content.height
    dock: Dock.Bottom
    modal: true

    // A page of its own over the browsing page, which stays where it was under it.
    function openPage(page) {
        hide()
        pageStack.push(Qt.resolvedUrl("../pages/" + page))
    }

    // What the sheet offers is for the page in front, and another page in front -- a
    // new tab from the cover, say -- makes it out of date.
    Connections {
        target: TabModel
        onActiveTabChanged: menu.hide()
    }

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

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)
    }

    Column {
        id: content

        width: parent.width
        bottomPadding: Theme.paddingMedium

        Item {
            width: parent.width
            height: Theme.paddingLarge

            DragHandle {
                objectName: "menuDragHandle"
                x: (parent.width - width) / 2
                y: Theme.paddingSmall
            }
        }

        SectionHeader {
            text: qsTr("Tabs")
        }

        Grid {
            width: parent.width
            columns: 4

            MenuButton {
                objectName: "newTabMenuButton"
                width: menu.width / 4
                iconSource: "image://theme/icon-m-tab-new"
                text: qsTr("New tab")
                onClicked: {
                    menu.hide()
                    TabModel.newTab(Settings.homePage)
                }
            }
        }

        SectionHeader {
            text: qsTr("This page")
        }

        Grid {
            width: parent.width
            columns: 4

            MenuButton {
                objectName: "findMenuButton"
                width: menu.width / 4
                enabled: menu.hasPage && menu.view !== null
                iconSource: "image://theme/icon-m-search-on-page"
                text: qsTr("Search on page")
                onClicked: {
                    menu.hide()
                    menu.findRequested()
                }
            }

            MenuButton {
                objectName: "bookmarkMenuButton"
                width: menu.width / 4
                enabled: menu.hasPage
                checked: BookmarkModel.activeUrlBookmarked
                iconSource: checked ? "image://theme/icon-m-favorite-selected"
                                    : "image://theme/icon-m-favorite"
                text: checked ? qsTr("Remove bookmark") : qsTr("Bookmark this page")
                onClicked: {
                    menu.hide()
                    if (BookmarkModel.activeUrlBookmarked) {
                        BookmarkModel.removeByUrl(TabModel.activeUrl)
                    } else {
                        BookmarkModel.add(TabModel.activeUrl, TabModel.activeTitle,
                                          TabModel.activeFavicon)
                    }
                }
            }

            MenuButton {
                objectName: "shareMenuButton"
                width: menu.width / 4
                enabled: menu.hasPage
                iconSource: "image://theme/icon-m-share"
                text: qsTr("Share")
                onClicked: {
                    menu.hide()
                    shareAction.trigger()
                }
            }

            // On while the page in front is in its desktop version, whichever way it
            // got there: this switch, or the setting every page starts from. Set on the
            // view, as sailfish-browser sets it on its own
            // (apps/browser/qml/pages/components/PopUpMenuItem.qml), and the engine
            // loads the page again in the version asked for. The view keeps it for as
            // long as it lives; one given up past the limit of loaded pages comes back
            // as the setting says.
            MenuButton {
                objectName: "desktopMenuButton"
                width: menu.width / 4
                enabled: menu.hasPage && menu.view !== null
                checked: menu.view !== null && menu.view.desktopMode === true
                iconSource: "image://theme/icon-m-computer"
                text: qsTr("Desktop version")
                onClicked: {
                    menu.hide()
                    menu.view.desktopMode = !menu.view.desktopMode
                }
            }
        }

        SectionHeader {
            text: qsTr("Browser")
        }

        Grid {
            width: parent.width
            columns: 4

            MenuButton {
                objectName: "bookmarksMenuButton"
                width: menu.width / 4
                iconSource: "image://theme/icon-m-favorite-selected"
                text: qsTr("Bookmarks")
                onClicked: menu.openPage("BookmarksPage.qml")
            }

            MenuButton {
                objectName: "historyMenuButton"
                width: menu.width / 4
                iconSource: "image://theme/icon-m-history"
                text: qsTr("History")
                onClicked: menu.openPage("HistoryPage.qml")
            }

            MenuButton {
                objectName: "downloadsMenuButton"
                width: menu.width / 4
                iconSource: "image://theme/icon-m-downloads"
                text: qsTr("Downloads")
                onClicked: menu.openPage("DownloadsPage.qml")
            }

            MenuButton {
                objectName: "settingsMenuButton"
                width: menu.width / 4
                iconSource: "image://theme/icon-m-setting"
                text: qsTr("Settings")
                onClicked: menu.openPage("SettingsPage.qml")
            }
        }
    }
}
