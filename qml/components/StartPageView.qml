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
    readonly property bool landscape: width > height
    readonly property bool showsTiles: showsTopSites || showsBookmarks
    // Landscape: top sites over bookmarks in the left half, recent pages in the right one; a half
    // with nothing to show leaves the other the whole width. Tiles portrait-sized (Screen.width
    // is portrait's), at most StartPage::TopSiteLimit a row.
    readonly property bool halves: landscape && showsTiles && showsRecentPages
    readonly property real columnWidth: halves ? width / 2 : width
    readonly property int fittingTiles: Math.floor(columnWidth / (Screen.width / 4))
    readonly property int columns: landscape ? Math.max(1, Math.min(8, fittingTiles)) : 4
    // Tiles past the chosen rows hidden; Grid skips hidden items.
    readonly property int topSiteTiles: StartPageSettings.topSiteRows * columns

    signal openRequested(string url)
    signal newTabRequested(string url)

    objectName: "startPage"
    contentHeight: column.height

    // Grid, not Flow: a fractional column width never wraps. As tall as the taller half.
    Grid {
        id: column

        width: parent.width
        columns: startPage.halves ? 2 : 1
        topPadding: Theme.paddingLarge
        // Clear of nav bar reach, which takes presses for grid drag.
        bottomPadding: Theme.itemSizeSmall

        // Left half in landscape: top sites over bookmarks.
        Column {
            width: startPage.columnWidth
            visible: startPage.showsTiles

            Column {
                objectName: "topSitesSection"
                width: startPage.columnWidth
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
                            width: startPage.columnWidth / startPage.columns
                            visible: index < startPage.topSiteTiles
                            url: model.url
                            favicon: model.favicon
                            onClicked: startPage.openRequested(model.url)
                        }
                    }
                }
            }

            Column {
                objectName: "bookmarksSection"
                width: startPage.columnWidth
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
                            width: startPage.columnWidth / startPage.columns
                            url: model.url
                            title: model.title
                            favicon: model.favicon
                            onClicked: startPage.openRequested(model.url)
                        }
                    }
                }
            }
        }

        // Row menus keep to this half.
        MenuScope {
            objectName: "recentPagesSection"
            width: startPage.columnWidth
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
