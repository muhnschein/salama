// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The navigation bar as the tutorial draws it: the bar's colour, its controls where they
// sit on the real one, the handle along its top edge, and under it all the real bar's own
// gesture, BarGesture, so the drag the tutorial teaches is caught, measured and reached
// for exactly as it is on the browsing page (docs/DECISIONS/0034-tutorial.md). The
// address and the menu answer a tap by saying so, and the tutorial decides what that
// shows; back and reload are pictures of controls, and what the reach above the bar hands
// on to a page has no page to go to.
//
// While the address is edited, the bar is the field, as the real one is: from the edge of
// the screen to the menu, with what was typed in it.
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
    // The address or the menu was tapped.
    signal tapped(string region)

    // The address as a field, with this typed into it.
    property bool editing: false
    property string typed
    readonly property bool dragging: gestureArea.dragging
    // The middle of the address and of the menu button, in the bar's own coordinates:
    // where the tutorial's hint taps.
    readonly property point addressCentre: Qt.point(width / 2, height / 2)
    readonly property point menuCentre: Qt.point(menuIcon.x + menuIcon.width / 2, height / 2)

    height: Theme.itemSizeLarge

    // As the real bar's: the menu from its icon to the edge, back and reload their icons'
    // room, and the address between.
    function regionAt(x) {
        if (x >= menuIcon.x) {
            return "menu"
        }
        if (x < backIcon.x + backIcon.width + Theme.paddingMedium
                || x >= reloadIcon.x - Theme.paddingMedium) {
            return ""
        }
        return "address"
    }

    function activate(region) {
        if (region.length > 0) {
            tapped(region)
        }
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
        id: backIcon

        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        visible: !tutorialBar.editing
        source: "image://theme/icon-m-back"
    }

    // The front tab's host, as the bar shows it, and on a first start "Search or enter
    // address", as the bar says then. Centred on the screen, as wide as the room on the
    // side with two controls allows.
    AddressLabel {
        anchors.centerIn: parent
        visible: !tutorialBar.editing
        url: TabModel.activeUrl
        pressed: gestureArea.pressedRegion === "address"
        maximumWidth: parent.width - 2 * (Theme.horizontalPageMargin + 2 * Theme.iconSizeMedium
                                          + Theme.paddingLarge + Theme.paddingMedium)
    }

    // The field, drawn: what was typed, from the edge to the menu, over the line Silica
    // draws under a field.
    Item {
        objectName: "tutorialField"
        x: Theme.paddingMedium
        width: menuIcon.x - Theme.paddingMedium - x
        height: parent.height
        visible: tutorialBar.editing

        Label {
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: Theme.paddingMedium
                verticalCenter: parent.verticalCenter
            }
            text: tutorialBar.typed
            textFormat: Text.PlainText
            truncationMode: TruncationMode.Fade
        }

        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                bottomMargin: Theme.paddingLarge
            }
            height: Theme._lineWidth
            color: Theme.highlightColor
        }
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
        highlighted: gestureArea.pressedRegion === "menu"
    }

    Icon {
        id: reloadIcon

        anchors {
            right: menuIcon.left
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        visible: !tutorialBar.editing
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
