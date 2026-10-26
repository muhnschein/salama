// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Half-size new-tab picture; empty sections drawn as faint placeholders. Takes no input.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // Settings page size = browsing page size, in either orientation.
    property real screenWidth
    property real screenHeight
    // Half the screen, or less where it is given less width (a column on landscape), with the
    // page margin either side.
    readonly property real miniature: Math.min(0.5, (width - 2 * Theme.horizontalPageMargin)
                                                    / screenWidth)
    readonly property bool blank: StartPageSettings.blank
    readonly property bool showsTopSites: !blank && StartPageSettings.topSites
    readonly property bool showsBookmarks: !blank && StartPageSettings.bookmarks
    readonly property bool showsRecentPages: !blank && StartPageSettings.recent
    // Landscape as on the start page: top sites over bookmarks in the left half, recent pages in
    // the right one, a half with nothing to show leaving the other the whole width; tiles
    // portrait-sized. Empty sections still drawn here, so only the switches count.
    readonly property bool landscape: screenWidth > screenHeight
    readonly property bool showsTiles: showsTopSites || showsBookmarks
    readonly property bool halves: landscape && showsTiles && showsRecentPages
    readonly property real columnWidth: halves ? screenWidth / 2 : screenWidth
    readonly property int fittingTiles: Math.floor(columnWidth / (Screen.width / 4))
    readonly property int columns: landscape ? Math.max(1, Math.min(8, fittingTiles)) : 4
    readonly property int topSiteTiles: StartPageSettings.topSiteRows * columns
    readonly property color ink: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)

    objectName: "startPagePreview"
    height: frame.height

    // As start page: counted in while shown.
    Component.onCompleted: StartPage.watch()
    Component.onDestruction: StartPage.unwatch()

    Rectangle {
        id: frame

        objectName: "startPagePreviewFrame"
        anchors.horizontalCenter: parent.horizontalCenter
        width: preview.screenWidth * preview.miniature
        height: preview.screenHeight * preview.miniature
        radius: Theme.paddingLarge
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityLow)
        border {
            width: Theme._lineWidth
            color: Theme.rgba(Theme.primaryColor, Theme.opacityLow)
        }

        // Clipping is always rectangular, so corners cut by mask.
        Item {
            id: glass

            anchors {
                fill: parent
                margins: frame.border.width
            }
            layer.enabled: true
            layer.effect: OpacityMask {
                maskSource: Rectangle {
                    width: glass.width
                    height: glass.height
                    radius: frame.radius - frame.border.width
                    visible: false
                }
            }

            // Full size scaled down so layout matches real start page.
            Item {
                id: screen

                objectName: "startPagePreviewScreen"
                width: preview.screenWidth
                height: preview.screenHeight
                scale: preview.miniature
                transformOrigin: Item.TopLeft
                enabled: false

                Grid {
                    width: parent.width
                    columns: preview.halves ? 2 : 1
                    topPadding: Theme.paddingLarge

                    Column {
                        width: preview.columnWidth
                        visible: preview.showsTiles

                        Column {
                            objectName: "startPagePreviewTopSites"
                            width: parent.width
                            visible: preview.showsTopSites

                            SectionHeader {
                                text: qsTr("Frequently visited")
                            }

                            Grid {
                                columns: preview.columns

                                Repeater {
                                    model: StartPage.topSites

                                    SiteTile {
                                        objectName: "startPagePreviewTile"
                                        width: preview.columnWidth / preview.columns
                                        visible: index < preview.topSiteTiles
                                        url: model.url
                                        favicon: model.favicon
                                    }
                                }

                                Repeater {
                                    model: StartPage.topSites.count > 0 ? 0 : preview.topSiteTiles

                                    SiteTile {
                                        objectName: "startPagePreviewSpareTile"
                                        width: preview.columnWidth / preview.columns

                                        Rectangle {
                                            anchors {
                                                horizontalCenter: parent.horizontalCenter
                                                bottom: parent.bottom
                                                bottomMargin: Theme.paddingMedium
                                                              + Theme.fontSizeExtraSmall / 2
                                            }
                                            width: Theme.itemSizeMedium * (0.6 + 0.1 * (index % 3))
                                            height: Theme.paddingSmall
                                            radius: height / 2
                                            color: preview.ink
                                        }
                                    }
                                }
                            }
                        }

                        Column {
                            objectName: "startPagePreviewBookmarks"
                            width: parent.width
                            visible: preview.showsBookmarks

                            SectionHeader {
                                text: qsTr("Bookmarks")
                            }

                            Grid {
                                columns: preview.columns

                                Repeater {
                                    model: StartPage.bookmarks

                                    SiteTile {
                                        objectName: "startPagePreviewTile"
                                        width: preview.columnWidth / preview.columns
                                        visible: index < preview.topSiteTiles
                                        url: model.url
                                        title: model.title
                                        favicon: model.favicon
                                    }
                                }

                                Repeater {
                                    model: StartPage.bookmarks.count > 0 ? 0 : preview.columns

                                    SiteTile {
                                        objectName: "startPagePreviewSpareTile"
                                        width: preview.columnWidth / preview.columns

                                        Rectangle {
                                            anchors {
                                                horizontalCenter: parent.horizontalCenter
                                                bottom: parent.bottom
                                                bottomMargin: Theme.paddingMedium
                                                              + Theme.fontSizeExtraSmall / 2
                                            }
                                            width: Theme.itemSizeMedium * (0.8 - 0.1 * (index % 2))
                                            height: Theme.paddingSmall
                                            radius: height / 2
                                            color: preview.ink
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Column {
                        objectName: "startPagePreviewRecent"
                        width: preview.columnWidth
                        visible: preview.showsRecentPages

                        SectionHeader {
                            text: qsTr("Recently visited")
                        }

                        Repeater {
                            model: StartPage.recentPages

                            TabRow {
                                objectName: "startPagePreviewRow"
                                width: preview.columnWidth
                                title: model.title
                                subtitle: model.url
                                icon: model.favicon
                            }
                        }

                        Repeater {
                            model: StartPage.recentPages.count > 0 ? 0 : 3

                            Item {
                                objectName: "startPagePreviewSpareRow"
                                width: preview.columnWidth
                                height: Theme.itemSizeMedium

                                Rectangle {
                                    id: spareIcon

                                    anchors {
                                        left: parent.left
                                        leftMargin: Theme.horizontalPageMargin
                                        verticalCenter: parent.verticalCenter
                                    }
                                    width: Theme.iconSizeSmall
                                    height: width
                                    radius: Theme.paddingSmall
                                    color: preview.ink
                                }

                                Column {
                                    anchors {
                                        left: spareIcon.right
                                        leftMargin: Theme.paddingMedium
                                        verticalCenter: parent.verticalCenter
                                    }
                                    spacing: Theme.paddingMedium

                                    Rectangle {
                                        width: preview.columnWidth * (0.5 - 0.08 * index)
                                        height: Theme.paddingMedium
                                        radius: height / 2
                                        color: preview.ink
                                    }

                                    Rectangle {
                                        width: preview.columnWidth * (0.3 + 0.05 * index)
                                        height: Theme.paddingSmall
                                        radius: height / 2
                                        color: preview.ink
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    objectName: "startPagePreviewBar"
                    anchors.bottom: parent.bottom
                    width: parent.width
                    // Landscape bar slim, as on the browsing page.
                    height: preview.landscape ? Theme.itemSizeSmall : Theme.itemSizeLarge
                    color: Theme.highlightDimmerColor

                    DragHandle {
                        x: (parent.width - width) / 2
                        y: -height / 2
                    }

                    Icon {
                        anchors {
                            left: parent.left
                            leftMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        source: "image://theme/icon-m-back"
                        opacity: Theme.opacityLow
                    }

                    AddressLabel {
                        anchors.centerIn: parent
                        maximumWidth: parent.width - 2 * (Theme.horizontalPageMargin
                                                          + 2 * Theme.iconSizeMedium
                                                          + Theme.paddingLarge)
                    }

                    Icon {
                        id: barMenu

                        anchors {
                            right: parent.right
                            rightMargin: Theme.horizontalPageMargin
                            verticalCenter: parent.verticalCenter
                        }
                        source: "image://theme/icon-m-menu"
                    }

                    Icon {
                        anchors {
                            right: barMenu.left
                            rightMargin: Theme.paddingLarge
                            verticalCenter: parent.verticalCenter
                        }
                        source: "image://theme/icon-m-refresh"
                    }
                }
            }
        }
    }
}
