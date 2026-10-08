// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Menu entry: icon over name; page actions on lit disc (round), busy ring for progress.
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
    readonly property bool lit: highlighted || checked

    height: column.height + 2 * Theme.paddingMedium
    opacity: enabled ? 1.0 : Theme.opacityLow
    highlightedColor: "transparent"

    Column {
        id: column

        anchors.centerIn: parent
        width: parent.width
        spacing: Theme.paddingSmall

        // Same height with or without disc so names line up.
        Item {
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

        // No padding: five per row, "Desktop site" in tiny type needs nearly full width.
        Label {
            objectName: "menuButtonLabel"
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: button.text
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeTiny
            color: button.lit ? Theme.highlightColor : Theme.primaryColor
        }
    }
}
