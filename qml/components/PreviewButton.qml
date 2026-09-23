// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// A button drawn over a tab's preview: a disc of the highlight colour with a mark on
// it, faint enough not to be the first thing seen on each cell and opaque under a
// finger. The disc is what keeps the mark readable over a white page and a dark one
// alike (docs/DECISIONS/0010-tab-grid-deck.md). The close button in a preview's corner
// is one, and so are the media controls along its foot.
//
// It takes its own presses, above the handler the cell's gestures go through, so a
// tap on it is never a tap on the cell.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: button

    // Its own tap signal: the handler's carries a mouse event, which a test cannot give
    // it.
    signal clicked()

    // What the disc is called, for the tests that look for it.
    property string markName
    readonly property bool pressed: tap.pressed
    // What is drawn on the disc.
    default property alias mark: disc.data

    // The touch target is larger than the disc; the disc is what shows.
    width: Theme.iconSizeMedium + Theme.paddingSmall
    height: width

    MouseArea {
        id: tap

        anchors.fill: parent
        onClicked: button.clicked()
    }

    Rectangle {
        id: disc

        objectName: button.markName
        anchors.centerIn: parent
        width: Theme.iconSizeSmall + Theme.paddingMedium
        height: width
        radius: width / 2
        color: tap.pressed ? Theme.highlightColor : Theme.highlightBackgroundColor
        opacity: tap.pressed ? 1.0 : Theme.opacityHigh
    }
}
