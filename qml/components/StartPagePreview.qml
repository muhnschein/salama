// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A picture of what a new tab shows, on the start page's settings, under the switches it
// follows (pages/StartPageSettingsPage.qml, docs/DECISIONS/0032-start-page.md): the
// phone's screen, half its size, as the start page lays it out -- the sections switched
// on, in its order, under their headings, made of the start page's own tiles and rows
// with the sites the reader has, and the navigation bar along the foot as a new tab has
// it. A section with nothing in it yet is drawn as where things go, faint squares and
// lines, since what is being set is what a tab will show; a blank page, or none of the
// sections, is the bar alone. Not a button: nothing in it answers a touch.
import QtQuick 2.6
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // The screen the picture is of: the settings page's size, which in portrait is the
    // browsing page's.
    property real screenWidth
    property real screenHeight
    readonly property real miniature: 0.5
    readonly property bool blank: StartPageSettings.blank
    readonly property bool showsTopSites: !blank && StartPageSettings.topSites
    readonly property bool showsBookmarks: !blank && StartPageSettings.bookmarks
    readonly property bool showsRecentPages: !blank && StartPageSettings.recent
    // The start page's four to a row (StartPageView).
    readonly property int columns: 4
    readonly property color ink: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)

    objectName: "startPagePreview"
    height: frame.height

    // The screen's edge, round its corners as a phone's are.
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

        // Cut to the frame's corners: clipping is rectangular whatever the shape of the
        // item doing it, so the corners are cut by a mask, as the grid's cells are
        // (docs/DECISIONS/0010-tab-grid-deck.md).
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

            // The screen at its own size, made smaller: the start page's own measures,
            // tiles and type, so the picture is laid out as a new tab is.
            Item {
                id: screen

                width: preview.screenWidth
                height: preview.screenHeight
                scale: preview.miniature
                transformOrigin: Item.TopLeft
                enabled: false

                Column {
                    width: parent.width
                    topPadding: Theme.paddingLarge

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
                                    width: screen.width / preview.columns
                                    url: model.url
                                    favicon: model.favicon
                                }
                            }

                            Repeater {
                                model: StartPage.topSites.count > 0 ? 0 : preview.columns

                                SiteTile {
                                    objectName: "startPagePreviewSpareTile"
                                    width: screen.width / preview.columns

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
                                    width: screen.width / preview.columns
                                    url: model.url
                                    title: model.title
                                    favicon: model.favicon
                                }
                            }

                            Repeater {
                                model: StartPage.bookmarks.count > 0 ? 0 : preview.columns

                                SiteTile {
                                    objectName: "startPagePreviewSpareTile"
                                    width: screen.width / preview.columns

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

                    Column {
                        objectName: "startPagePreviewRecent"
                        width: parent.width
                        visible: preview.showsRecentPages

                        SectionHeader {
                            text: qsTr("Recently visited")
                        }

                        Repeater {
                            model: StartPage.recentPages

                            TabRow {
                                objectName: "startPagePreviewRow"
                                width: screen.width
                                title: model.title
                                subtitle: model.url
                                icon: model.favicon
                            }
                        }

                        Repeater {
                            model: StartPage.recentPages.count > 0 ? 0 : 3

                            Item {
                                objectName: "startPagePreviewSpareRow"
                                width: screen.width
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
                                        width: screen.width * (0.5 - 0.08 * index)
                                        height: Theme.paddingMedium
                                        radius: height / 2
                                        color: preview.ink
                                    }

                                    Rectangle {
                                        width: screen.width * (0.3 + 0.05 * index)
                                        height: Theme.paddingSmall
                                        radius: height / 2
                                        color: preview.ink
                                    }
                                }
                            }
                        }
                    }
                }

                // The navigation bar as a new tab has it: nothing to go back to, and
                // where the address will be, the words that ask for one.
                Rectangle {
                    objectName: "startPagePreviewBar"
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: Theme.itemSizeLarge
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
