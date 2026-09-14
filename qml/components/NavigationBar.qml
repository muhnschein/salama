// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The bar along the bottom of the browsing page: back, the address, reload/stop and
// the menu. Dragging it upwards pulls the tab grid up from underneath the page.
//
// One MouseArea covers the whole bar and owns every press, and the icons are just
// icons. A drag has to be recognised from the press that starts it, and the controls
// tile the bar edge to edge: a handler behind them is never reached, and one that
// lets presses through to them cannot see the movement afterwards. So the press is
// taken here, the region under it decides what a tap means, and the same region
// drives the pressed highlight.
//
// The drag is reported as a distance, not as a finished gesture: the page follows the
// finger while it moves and decides when it lifts. A handler that only speaks at its
// threshold shows nothing while the finger is down, and on device that is
// indistinguishable from the system's own bottom-edge swipe having taken the touch.
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
    // Upward drag, in pixels from where the finger went down. Negative means it has
    // come back below its own starting point.
    signal dragStarted()
    signal dragMoved(real distance)
    signal dragFinished(real distance)

    // The bar is the whole touch target for the drag, and it sits in the strip the
    // system watches for its own edge swipe. Every bit of height here is height the
    // gesture can start in without lipstick taking it first.
    height: Theme.itemSizeLarge

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

    // The page runs the full height of the window and this bar lies over its foot,
    // so the last line of a page is dimmed rather than cut off.
    Rectangle {
        objectName: "navigationBarBackground"
        anchors.fill: parent
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityOverlay)
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
        property real distance: 0
        property bool dragging: false
        property string pressedRegion: ""

        objectName: "navigationBarGesture"
        anchors.fill: parent
        enabled: !navigationBar.editing

        onPressed: {
            pressedY = mouse.y
            distance = 0
            dragging = false
            pressedRegion = navigationBar.regionAt(mouse.x)
        }
        onPositionChanged: {
            distance = pressedY - mouse.y
            // Theme.startDragDistance is the movement Silica treats as a drag rather
            // than a shaky tap; past it the press belongs to the page, not a control.
            if (!dragging && distance > Theme.startDragDistance) {
                dragging = true
                pressedRegion = ""
                navigationBar.dragStarted()
            }
            if (dragging) {
                navigationBar.dragMoved(distance)
            }
        }
        onReleased: {
            if (dragging) {
                navigationBar.dragFinished(distance)
            }
            pressedRegion = ""
        }
        // The grab can be taken away mid-drag -- by the system's edge gesture, most
        // of all. Finish at zero so the page springs back rather than hanging.
        onCanceled: {
            if (dragging) {
                navigationBar.dragFinished(0)
            }
            dragging = false
            pressedRegion = ""
        }
        onClicked: {
            if (!dragging) {
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
