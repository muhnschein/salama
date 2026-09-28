// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A tab group's picture in the list of groups: the previews of its most recent tabs, two
// by two in a small square, the most recent at the top left, from the previews the grid
// already has (docs/DECISIONS/0008-tab-previews.md). Square at the corners, pictures and
// frame alike, as Silica's own pictures in a list are -- the Gallery's albums, a
// contact's avatar. A tab with no preview is the grid's placeholder ground, a place with
// no tab at all a fainter one, and a group with no tabs is the square's outline alone,
// dashed. The current group is framed in the highlight background colour just outside
// the picture, the colour the grid frames the tab in front in
// (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: collage

    // The pictures' paths, the most recent tab's first, as TabGroups' previews role hands
    // them out: four at most, an empty string for a tab with none.
    property var previews: []
    // Whether this is the group the grid shows.
    property bool current: false
    // Between two pictures, and the width of the current group's frame.
    readonly property real gap: Theme.paddingSmall / 2
    readonly property int tabCount: previews ? previews.length : 0

    width: Theme.iconSizeLarge
    height: width

    Rectangle {
        objectName: "tabGroupCollageFrame"
        anchors {
            fill: parent
            margins: -collage.gap
        }
        color: "transparent"
        border {
            width: collage.gap
            color: Theme.highlightBackgroundColor
        }
        visible: collage.current
    }

    Item {
        objectName: "tabGroupCollagePicture"
        anchors.fill: parent
        visible: collage.tabCount > 0

        Grid {
            columns: 2
            spacing: collage.gap

            Repeater {
                // Two by two, TabGroupModel::PreviewLimit.
                model: 4

                Rectangle {
                    readonly property bool holdsTab: index < collage.tabCount
                    readonly property string path: holdsTab ? collage.previews[index] : ""

                    objectName: "tabGroupCollageCell"
                    width: (collage.width - collage.gap) / 2
                    height: width
                    color: holdsTab ? Theme.rgba(Theme.highlightBackgroundColor,
                                                 Theme.highlightBackgroundOpacity)
                                    : Theme.rgba(Theme.primaryColor, Theme.opacityFaint / 2)

                    // Cropped to the top of the picture, as the grid shows the top of
                    // what was last on the screen rather than the middle of the page.
                    // Decoded at the size it is drawn: a preview is half the screen.
                    Image {
                        objectName: "tabGroupCollageImage"
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        verticalAlignment: Image.AlignTop
                        sourceSize.width: width
                        asynchronous: true
                        source: parent.path.length > 0 ? "file://" + parent.path : ""
                        visible: status === Image.Ready
                    }
                }
            }
        }
    }

    // A group with no tabs: the square's outline in dashes. Qt Quick draws no dashed
    // border, and Canvas's dashes came after Qt 5.6, so each side is a row of short bars,
    // the one row turned a quarter further for each side, reaching as near its corners as
    // whole dashes do.
    Item {
        id: outline

        readonly property real dash: Theme.paddingSmall
        readonly property int dashes: Math.floor((collage.width + dash) / (2 * dash))

        objectName: "tabGroupCollageOutline"
        anchors.fill: parent
        visible: collage.tabCount === 0

        Repeater {
            model: 4

            Item {
                width: collage.width
                height: collage.height
                rotation: index * 90

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: outline.dash

                    Repeater {
                        model: outline.dashes

                        Rectangle {
                            width: outline.dash
                            height: Theme._lineWidth
                            color: Theme.rgba(Theme.primaryColor, Theme.opacityLow)
                        }
                    }
                }
            }
        }
    }
}
