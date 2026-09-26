// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The navigation bar as the tutorial draws it: the bar's colour, its controls where
// they sit on the real one, the handle along its top edge, and under it all the real
// bar's own gesture, BarGesture, so the drag the tutorial teaches is caught, measured
// and reached for exactly as it is on the browsing page (docs/DECISIONS/0034-tutorial.md).
// The controls are pictures of controls: a tap on one does nothing, and what the reach
// above the bar hands on to a page has no page to go to.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: tutorialBar

    // What BarGesture raises on the bar it is the gesture of.
    signal dragStarted()
    signal dragMoved(real distance)
    signal dragFinished(real distance)
    signal pageTouchStarted(point position)
    signal pageTouchMoved(point position)
    signal pageTouchEnded(point position)

    readonly property bool dragging: gestureArea.dragging

    height: Theme.itemSizeLarge

    // Every press is the drag's or nothing's: there is only the one region.
    function regionAt(x) {
        return ""
    }

    function activate(region) {
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    DragHandle {
        objectName: "tutorialDragHandle"
        x: (tutorialBar.width - width) / 2
        y: -height / 2
        active: gestureArea.pressed && !gestureArea.forwarding || gestureArea.dragging
    }

    Icon {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: "image://theme/icon-m-back"
    }

    // The front tab's host, as the bar shows it: the tutorial is a picture of this
    // browser, and on a first start "Search or enter address", as the bar says then.
    // Centred on the screen, as wide as the room on the side with two controls allows.
    AddressLabel {
        anchors.centerIn: parent
        url: TabModel.activeUrl
        maximumWidth: parent.width - 2 * (Theme.horizontalPageMargin + 2 * Theme.iconSizeMedium
                                          + Theme.paddingLarge + Theme.paddingMedium)
    }

    Icon {
        id: menuIcon

        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: "image://theme/icon-m-menu"
    }

    Icon {
        anchors {
            right: menuIcon.left
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: "image://theme/icon-m-refresh"
    }

    BarGesture {
        id: gestureArea

        objectName: "tutorialBarGesture"
        bar: tutorialBar
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: -reach
        }
    }
}
