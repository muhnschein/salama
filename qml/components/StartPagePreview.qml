// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A picture of what a new tab shows, on the start page's settings, under the switches it
// follows (pages/StartPageSettingsPage.qml, docs/DECISIONS/0032-start-page.md): the
// sections switched on, in the start page's order, drawn as where things go rather than
// as what they are -- squares and a bar under each for the sites visited most, a heading
// and a row of cells for the bookmarks, a heading and two lines for the pages read last --
// and with none of them, or a blank page, a line saying so. The sections show whether or
// not there is anything in them yet, since what is being set is what a tab will show.
// Not a button.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Column {
    id: preview

    readonly property bool blank: StartPageSettings.blank
    readonly property bool showsTopSites: !blank && StartPageSettings.topSites
    readonly property bool showsBookmarks: !blank && StartPageSettings.bookmarks
    readonly property bool showsRecentPages: !blank && StartPageSettings.recent
    readonly property bool empty: !showsTopSites && !showsBookmarks && !showsRecentPages
    // Five to a row: the page's own four, narrowed to fit a picture of it.
    readonly property int columns: 5
    readonly property real cell: (sketch.width - 2 * Theme.paddingLarge
                                  - (columns - 1) * Theme.paddingMedium) / columns
    readonly property color ink: Theme.rgba(Theme.primaryColor, Theme.opacityLow)
    readonly property color heading: Theme.secondaryHighlightColor

    objectName: "startPagePreview"
    spacing: Theme.paddingSmall

    Rectangle {
        id: sketch

        objectName: "startPagePreviewSketch"
        width: parent.width
        height: Theme.itemSizeExtraLarge * 1.8
        radius: Theme.paddingSmall
        clip: true
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityHigh)

        Column {
            x: Theme.paddingLarge
            y: Theme.paddingLarge
            width: parent.width - 2 * x
            spacing: Theme.paddingLarge

            // The sites visited most: a square each, a bar for its name under it.
            Row {
                objectName: "startPagePreviewTopSites"
                visible: preview.showsTopSites
                spacing: Theme.paddingMedium

                Repeater {
                    model: preview.columns

                    Column {
                        width: preview.cell
                        spacing: Theme.paddingSmall

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.min(preview.cell, Theme.iconSizeSmallPlus)
                            height: width
                            radius: Theme.paddingSmall
                            color: Theme.rgba(Theme.highlightBackgroundColor,
                                              Theme.highlightBackgroundOpacity)
                        }

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.min(preview.cell, Theme.iconSizeSmallPlus)
                                   * (0.75 + 0.05 * (index % 3))
                            height: Theme.paddingSmall / 2
                            radius: height / 2
                            color: preview.ink
                        }
                    }
                }
            }

            // The bookmarks: a heading, and a row of cells.
            Column {
                objectName: "startPagePreviewBookmarks"
                width: parent.width
                visible: preview.showsBookmarks
                spacing: Theme.paddingSmall

                Rectangle {
                    width: parent.width / 4
                    height: Theme.paddingSmall * 2 / 3
                    radius: height / 2
                    color: preview.heading
                }

                Row {
                    spacing: Theme.paddingMedium

                    Repeater {
                        model: preview.columns

                        Rectangle {
                            width: preview.cell
                            height: Theme.iconSizeSmall
                            radius: Theme.paddingSmall
                            color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                        }
                    }
                }
            }

            // The pages read last: a heading, and a line each for two of them.
            Column {
                objectName: "startPagePreviewRecent"
                width: parent.width
                visible: preview.showsRecentPages
                spacing: Theme.paddingSmall

                Rectangle {
                    width: parent.width / 3
                    height: Theme.paddingSmall * 2 / 3
                    radius: height / 2
                    color: preview.heading
                }

                Repeater {
                    model: 2

                    Row {
                        spacing: Theme.paddingMedium

                        Rectangle {
                            width: Theme.iconSizeExtraSmall
                            height: width
                            radius: Theme.paddingSmall / 2
                            color: preview.ink
                        }

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: preview.cell * (index === 0 ? 3 : 2.4)
                            height: Theme.paddingSmall / 2
                            radius: height / 2
                            color: preview.ink
                        }
                    }
                }
            }
        }

        Label {
            objectName: "startPagePreviewEmpty"
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingLarge
            visible: preview.empty
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            //: What a new tab shows with the start page blank or every section off
            text: qsTr("Nothing but the address bar")
        }
    }

    Label {
        objectName: "startPagePreviewCaption"
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.secondaryHighlightColor
        //: Under a picture of what a new tab will show
        text: qsTr("Preview")
    }
}
