// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The bar along the bottom of the browsing page: back, the address, reload/stop and
// the menu. Dragging it upwards opens the tab grid.
//
// One MouseArea covers the whole bar and owns every press, and the icons are just
// icons. A drag has to be recognised from the press that starts it, and the controls
// tile the bar edge to edge: a handler behind them is never reached, and one that
// lets presses through to them cannot see the movement afterwards. So the press is
// taken here, the region under it decides what a tap means, and the same region
// drives the pressed highlight.
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

    height: Theme.itemSizeMedium

    function beginEditing() {
        urlField.text = navigationBar.url
        editing = true
        urlField.forceActiveFocus()
        urlField.selectAll()
    }

    function endEditing() {
        editing = false
    }

    function submit() {
        var text = urlField.text
        endEditing()
        if (text.length > 0) {
            navigationBar.accepted(text)
        }
    }

    // True when an upward drag is long enough to count as opening the tab grid.
    // A named function rather than a condition inside the handler so the threshold
    // is reachable from the load tests, which have no window to send presses to.
    function isPullUp(startY, currentY) {
        return startY - currentY > Theme.itemSizeSmall
    }

    // Which control a press at this x belongs to. Named regions rather than hit
    // testing: the gesture handler sits on top of everything, so childAt() would
    // only ever return the handler itself.
    function regionAt(x) {
        if (x < addressArea.x) {
            return "back"
        }
        if (x >= menuIcon.x) {
            return "menu"
        }
        if (x >= reloadIcon.x) {
            return "reload"
        }
        return "address"
    }

    function activate(region) {
        if (region === "back") {
            if (navigationBar.canGoBack) {
                navigationBar.back()
            }
        } else if (region === "menu") {
            navigationBar.showMenu()
        } else if (region === "reload") {
            if (navigationBar.loading) {
                navigationBar.stop()
            } else {
                navigationBar.reload()
            }
        } else if (region === "address") {
            navigationBar.beginEditing()
        }
    }

    Icon {
        id: backIcon

        objectName: "backButton"
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: "image://theme/icon-m-back"
        opacity: navigationBar.canGoBack ? 1.0 : Theme.highlightBackgroundOpacity
        highlighted: gestureArea.pressedRegion === "back"
    }

    Icon {
        id: menuIcon

        objectName: "menuButton"
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

        objectName: "reloadButton"
        anchors {
            right: menuIcon.left
            rightMargin: Theme.paddingLarge
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        source: navigationBar.loading ? "image://theme/icon-m-clear"
                                      : "image://theme/icon-m-refresh"
        highlighted: gestureArea.pressedRegion === "reload"
    }

    Item {
        id: addressArea

        anchors {
            left: backIcon.right
            leftMargin: Theme.paddingLarge
            right: reloadIcon.left
            rightMargin: Theme.paddingLarge
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
            color: {
                if (gestureArea.pressedRegion === "address") {
                    return Theme.highlightColor
                }
                return navigationBar.privateTab ? Theme.highlightColor : Theme.primaryColor
            }
            font.pixelSize: Theme.fontSizeSmall
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

    // Every press on the bar, so a drag is seen from the start. Stands down while
    // the address is being edited: the field needs its own taps for the caret.
    MouseArea {
        id: gestureArea

        property real pressedY: 0
        property bool dragged: false
        property string pressedRegion: ""

        objectName: "navigationBarGesture"
        anchors.fill: parent
        enabled: !navigationBar.editing

        onPressed: {
            pressedY = mouse.y
            dragged = false
            pressedRegion = navigationBar.regionAt(mouse.x)
        }
        onPositionChanged: {
            if (!dragged && navigationBar.isPullUp(pressedY, mouse.y)) {
                dragged = true
                pressedRegion = ""
                navigationBar.pullUp()
            }
        }
        onReleased: pressedRegion = ""
        onCanceled: {
            dragged = false
            pressedRegion = ""
        }
        onClicked: {
            if (!dragged) {
                navigationBar.activate(navigationBar.regionAt(mouse.x))
            }
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
