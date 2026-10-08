// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

SilicaFlickable {
    id: startPage

    readonly property bool blank: StartPageSettings.blank
    readonly property bool showsTopSites: !blank && StartPageSettings.topSites
                                          && StartPage.topSites.count > 0
    readonly property bool showsBookmarks: !blank && StartPageSettings.bookmarks
                                           && StartPage.bookmarks.count > 0
    readonly property bool showsRecentPages: !blank && StartPageSettings.recent
                                             && StartPage.recentPages.count > 0
    readonly property int columns: 4

    signal openRequested(string url)
    signal newTabRequested(string url)

    objectName: "startPage"
    contentHeight: column.height

    Column {
        id: column

        width: parent.width
        topPadding: Theme.paddingLarge
        // Clear of nav bar reach, which takes presses for grid drag.
        bottomPadding: Theme.itemSizeSmall

        Column {
            objectName: "topSitesSection"
            width: parent.width
            visible: startPage.showsTopSites

            SectionHeader {
                text: qsTr("Frequently visited")
            }

            Grid {
                width: parent.width
                columns: startPage.columns

                Repeater {
                    model: StartPage.topSites

                    SiteTile {
                        objectName: "topSiteTile"
                        width: startPage.width / startPage.columns
                        url: model.url
                        favicon: model.favicon
                        onClicked: startPage.openRequested(model.url)
                    }
                }
            }
        }

        Column {
            objectName: "bookmarksSection"
            width: parent.width
            visible: startPage.showsBookmarks

            SectionHeader {
                text: qsTr("Bookmarks")
            }

            Grid {
                width: parent.width
                columns: startPage.columns

                Repeater {
                    model: StartPage.bookmarks

                    SiteTile {
                        objectName: "bookmarkTile"
                        width: startPage.width / startPage.columns
                        url: model.url
                        title: model.title
                        favicon: model.favicon
                        onClicked: startPage.openRequested(model.url)
                    }
                }
            }
        }

        Column {
            objectName: "recentPagesSection"
            width: parent.width
            visible: startPage.showsRecentPages

            SectionHeader {
                text: qsTr("Recently visited")
            }

            Repeater {
                model: StartPage.recentPages

                TabRow {
                    id: recentRow

                    readonly property string pageUrl: model.url

                    objectName: "recentPageRow"
                    width: parent.width
                    title: model.title
                    subtitle: model.url
                    icon: model.favicon
                    onClicked: startPage.openRequested(pageUrl)
                    menu: ContextMenu {
                        MenuItem {
                            objectName: "recentPageNewTabMenu"
                            text: qsTr("Open in new tab")
                            onClicked: startPage.newTabRequested(recentRow.pageUrl)
                        }

                        MenuItem {
                            objectName: "recentPageRemoveMenu"
                            text: qsTr("Remove")
                            onClicked: HistoryModel.removeUrl(recentRow.pageUrl)
                        }
                    }
                }
            }
        }
    }

    ViewPlaceholder {
        objectName: "startPagePlaceholder"
        enabled: !startPage.blank && !startPage.showsTopSites && !startPage.showsBookmarks
                 && !startPage.showsRecentPages
        text: qsTr("Nothing here yet")
        hintText: qsTr("The sites you visit and bookmark show up here")
    }

    VerticalScrollDecorator {}
}
