// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0

Row {
    id: progress

    // current is 0-based.
    property int count
    property int current

    spacing: Theme.paddingMedium

    Repeater {
        model: progress.count

        Item {
            objectName: "tutorialProgressDot"
            width: Theme.paddingMedium
            height: width

            readonly property bool current: index === progress.current

            Rectangle {
                anchors.centerIn: parent
                width: parent.width * 2
                height: width
                radius: width / 2
                visible: parent.current
                color: Theme.rgba(Theme.highlightColor, Theme.opacityFaint)
            }

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: index < progress.current
                       ? Theme.secondaryHighlightColor
                       : parent.current ? Theme.highlightColor
                                        : Theme.rgba(Theme.primaryColor, Theme.opacityLow)
            }
        }
    }
}
