// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Which of the tutorial's lessons this is: a dot for each, the ones done in the
// secondary highlight, the one under way in the highlight colour with a glow round it,
// and the ones to come faint, under the words that say the step
// (pages/TutorialPage.qml, docs/DECISIONS/0034-tutorial.md). The Tutorial's own lessons
// are counted the same way. Not a button: the steps move on by the gestures they teach.
import QtQuick 2.6
import Sailfish.Silica 1.0

Row {
    id: progress

    // How many lessons there are, and which, from 0, is under way.
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

            // The glow, as Silica lights a switch that is on.
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
