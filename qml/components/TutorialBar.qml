// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Real BarGesture under sketched bar, so taught drag behaves exactly as on browsing page.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: tutorialBar

    signal dragArmed()
    signal dragDisarmed()
    signal dragStarted()
    signal dragMoved(real distance)
    signal dragFinished(real distance)
    signal pageTouchStarted(point position)
    signal pageTouchMoved(point position)
    signal pageTouchEnded(point position)
    signal tapped(string region)

    property bool editing: false
    property string typed
    readonly property bool dragging: gestureArea.dragging
    // Hint tap targets, bar coords.
    readonly property point addressCentre: Qt.point(width / 2, height / 2)
    readonly property point menuCentre: Qt.point(menuIcon.x + menuIcon.width / 2, height / 2)

    height: Theme.itemSizeLarge

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

    AddressLabel {
        anchors.centerIn: parent
        visible: !tutorialBar.editing
        url: TabModel.activeUrl
        pressed: gestureArea.pressedRegion === "address"
        maximumWidth: parent.width - 2 * (Theme.horizontalPageMargin + 2 * Theme.iconSizeMedium
                                          + Theme.paddingLarge + Theme.paddingMedium)
    }

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
