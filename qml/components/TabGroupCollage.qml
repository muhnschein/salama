// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: collage

    // Most recent first, max 4, "" for tab without preview.
    property var previews: []
    property bool current: false
    // Also current frame width.
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
                // TabGroupModel::PreviewLimit.
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

                    // Top-cropped; decoded at drawn size (preview is half screen).
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

    // No dashed borders in Qt Quick and Canvas dashes post-date Qt 5.6, so rows of bars.
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
