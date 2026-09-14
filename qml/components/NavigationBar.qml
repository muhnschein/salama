// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The bar along the bottom of the browsing page: back, the address, reload/stop and
// the menu. Dragging the bar upwards opens the tab grid; the drag handler sits behind
// the controls so a press that lands on one of them still reaches it.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: navigationBar

    property string url
    property bool privateTab: false
    property bool loading: false
    property int loadProgress: 0
    property bool canGoBack: false
    // The address turns into a field in place while it is being edited.
    property bool editing: false

    signal accepted(string text)
    signal back()
    signal reload()
    signal stop()
    signal showMenu()
    signal pullUp()

    height: Theme.itemSizeSmall

    function beginEditing() {
        urlField.text = navigationBar.url
        editing = true
        urlField.forceActiveFocus()
        urlField.selectAll()
    }

    function endEditing() {
        editing = false
    }

    // True when an upward drag is long enough to count as opening the tab grid. A
    // named function rather than a condition inside the handler so the threshold is
    // reachable from the load tests, which have no window to send real presses to.
    function isPullUp(startY, currentY) {
        return startY - currentY > Theme.itemSizeSmall
    }

    function submit() {
        var text = urlField.text
        endEditing()
        if (text.length > 0) {
            navigationBar.accepted(text)
        }
    }

    MouseArea {
        id: dragArea

        // Behind the controls: this only ever sees presses they did not take.
        property real pressedY: 0
        property bool armed: false

        objectName: "navigationBarDrag"
        anchors.fill: parent
        z: -1

        onPressed: {
            pressedY = mouse.y
            armed = true
        }
        onPositionChanged: {
            if (armed && navigationBar.isPullUp(pressedY, mouse.y)) {
                armed = false
                navigationBar.pullUp()
            }
        }
        onReleased: armed = false
        onCanceled: armed = false
    }

    IconButton {
        id: backButton

        objectName: "backButton"
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        height: parent.height
        icon.source: "image://theme/icon-m-back"
        enabled: navigationBar.canGoBack
        onClicked: navigationBar.back()
    }

    IconButton {
        id: menuButton

        objectName: "menuButton"
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        height: parent.height
        icon.source: "image://theme/icon-m-menu"
        onClicked: navigationBar.showMenu()
    }

    IconButton {
        id: reloadButton

        objectName: "reloadButton"
        anchors {
            right: menuButton.left
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        height: parent.height
        icon.source: navigationBar.loading ? "image://theme/icon-m-clear"
                                           : "image://theme/icon-m-refresh"
        onClicked: {
            if (navigationBar.loading) {
                navigationBar.stop()
            } else {
                navigationBar.reload()
            }
        }
    }

    Item {
        id: addressArea

        anchors {
            left: backButton.right
            right: reloadButton.left
            top: parent.top
            bottom: parent.bottom
        }

        Label {
            objectName: "addressLabel"
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: !navigationBar.editing
            text: navigationBar.url.length > 0 ? navigationBar.url
                                               : qsTr("Search or enter address")
            truncationMode: TruncationMode.Fade
            color: navigationBar.privateTab ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        MouseArea {
            objectName: "addressTapArea"
            anchors.fill: parent
            enabled: !navigationBar.editing
            onClicked: navigationBar.beginEditing()
        }

        TextField {
            id: urlField

            objectName: "addressField"
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }
            visible: navigationBar.editing
            label: navigationBar.privateTab ? qsTr("Private tab") : ""
            placeholderText: qsTr("Search or enter address")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
            EnterKey.enabled: text.length > 0
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: navigationBar.submit()
        }
    }

    Rectangle {
        objectName: "loadProgress"
        anchors {
            left: parent.left
            top: parent.top
        }
        height: Theme.paddingSmall
        width: parent.width * navigationBar.loadProgress / 100
        color: Theme.highlightColor
        visible: navigationBar.loading
    }
}
