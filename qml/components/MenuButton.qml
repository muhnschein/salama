// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Menu entry: icon over name; page actions on lit disc (round), busy ring for progress.
// Across: list row, icon at left, name beside it (landscape side sheet).
import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: button

    property string iconSource
    property string text
    property bool checked: false
    property bool round: false
    // progress 0..1.
    property bool busy: false
    property real progress: 0
    // Landscape rows are wider than portrait's cells.
    property real labelSize: Theme.fontSizeTiny
    property bool across: false
    readonly property bool lit: highlighted || checked

    // Across: Silica's list row height; holds portrait's disc.
    height: across ? Theme.itemSizeSmall : column.height + 2 * Theme.paddingMedium
    opacity: enabled ? 1.0 : Theme.opacityLow
    highlightedColor: "transparent"

    // Upright: as a Column, icon centred over name.
    Item {
        id: column

        anchors.centerIn: parent
        width: parent.width
        height: button.across ? Theme.itemSizeSmall
                              : iconCell.height + Theme.paddingSmall + label.height

        // Across: icon at page margin, as head's tile.
        Item {
            id: iconCell

            x: button.across ? Theme.horizontalPageMargin - (width - Theme.iconSizeSmallPlus) / 2
                             : 0
            y: button.across ? Math.round((parent.height - height) / 2) : 0
            width: button.across ? slot.width : parent.width
            height: slot.height

            // Same height with or without disc so names line up.
            Item {
                id: slot

                objectName: "menuButtonIconSlot"
                anchors.horizontalCenter: parent.horizontalCenter
                width: Theme.iconSizeMedium + Theme.paddingSmall
                height: width

                Rectangle {
                    objectName: "menuButtonDisc"
                    anchors.fill: parent
                    radius: width / 2
                    visible: button.round
                    color: button.lit ? Theme.rgba(Theme.highlightBackgroundColor,
                                                   Theme.highlightBackgroundOpacity)
                                      : Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                }

                ProgressCircle {
                    objectName: "menuButtonProgress"
                    anchors.centerIn: parent
                    width: Theme.iconSizeMedium
                    height: width
                    visible: button.busy
                    value: button.progress
                    progressColor: Theme.highlightColor
                    backgroundColor: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
                    borderWidth: Theme.paddingSmall / 2
                }

                Icon {
                    objectName: "menuButtonIcon"
                    anchors.centerIn: parent
                    source: button.iconSource
                    sourceSize: Qt.size(Theme.iconSizeSmallPlus, Theme.iconSizeSmallPlus)
                    highlighted: button.lit
                }
            }
        }

        // Upright, no padding: five per row, "Desktop site" in tiny type needs nearly full width.
        Label {
            id: label

            objectName: "menuButtonLabel"
            x: button.across ? iconCell.x + iconCell.width + Theme.paddingMedium : 0
            y: button.across ? Math.round((parent.height - height) / 2)
                             : iconCell.height + Theme.paddingSmall
            width: button.across ? parent.width - x - Theme.horizontalPageMargin : parent.width
            horizontalAlignment: button.across ? Text.AlignLeft : Text.AlignHCenter
            text: button.text
            truncationMode: TruncationMode.Fade
            font.pixelSize: button.labelSize
            color: button.lit ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
