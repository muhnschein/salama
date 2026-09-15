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
import harbour.tuuli 1.0

Item {
    id: navigationBar

    property string url
    property bool privateTab: false
    property bool loading: false
    property int loadProgress: 0
    property bool canGoBack: false
    // The page came over TLS and the engine is not satisfied with it: a bad
    // certificate, a broken chain, mixed content. Drawn as a red, open padlock.
    property bool tlsBroken: false
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
        if (!editing) {
            return
        }
        editing = false
        urlField.focus = false
    }

    // The field losing focus, and the keyboard going away, both end editing: tapping
    // the page while the field is up used to leave the bar in edit mode with nothing
    // to type into. Named functions rather than logic inside the handlers, so both can
    // be exercised from the load tests, which have no window to take focus in and no
    // input panel to close.
    function focusChanged(hasFocus) {
        if (!hasFocus) {
            endEditing()
        }
    }

    function keyboardVisibilityChanged(keyboardVisible) {
        if (!keyboardVisible) {
            endEditing()
        }
    }

    // Silica lays a field out with room for its label above the text and its underline
    // below it, so centring the item leaves the text itself off centre. The field
    // publishes the offset for exactly this; an engine whose field has none gives
    // undefined, and then the item's own centre is the best that can be done.
    function textCentringOffset(field) {
        var offset = field.textVerticalCenterOffset
        return offset === undefined ? 0 : offset
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
            if (!navigationBar.editing) {
                navigationBar.beginEditing()
            }
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
        // Above the gesture handler, so that the field keeps its own presses for the
        // caret while the rest of the bar stays live -- the controls still work and the
        // bar can still be dragged while an address is being typed. The label and the
        // warning accept no presses of their own and fall through to the handler.
        z: 1

        Row {
            id: addressRow

            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }
            // While editing, the field below says everything this row would.
            visible: !navigationBar.editing
            spacing: Theme.paddingSmall

            // The platform's own warning glyph in the error colour. There is no open
            // padlock in the icon set, and sailfish-browser draws this same icon for
            // this same state (apps/browser/qml/pages/components/ToolBar.qml).
            Icon {
                id: securityIcon

                objectName: "securityWarning"
                anchors.verticalCenter: parent.verticalCenter
                width: Theme.iconSizeSmall
                height: width
                visible: navigationBar.tlsBroken
                source: "image://theme/icon-s-filled-warning"
                color: Theme.errorColor
            }

            Label {
                objectName: "addressLabel"
                anchors.verticalCenter: parent.verticalCenter
                // Wide enough for the text and no wider, so the row centres on what is
                // actually drawn; never wider than the space between the controls.
                width: Math.min(implicitWidth, addressArea.width
                                - (securityIcon.visible ? securityIcon.width + addressRow.spacing
                                                        : 0))
                // The host, not the whole url (Settings.displayAddress). Tapping brings
                // the field up with every character of it back.
                text: navigationBar.url.length > 0 ? Settings.displayAddress(navigationBar.url)
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
        }

        TextField {
            id: urlField

            objectName: "addressField"
            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: navigationBar.textCentringOffset(urlField)
            }
            visible: navigationBar.editing
            label: navigationBar.privateTab ? qsTr("Private tab") : ""
            placeholderText: qsTr("Search or enter address")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
            EnterKey.enabled: text.length > 0
            EnterKey.iconSource: "image://theme/icon-m-enter-accept"
            EnterKey.onClicked: navigationBar.submit()
            onActiveFocusChanged: navigationBar.focusChanged(activeFocus)
        }
    }

    // The input panel closing is the other end of editing: the field can keep focus
    // after the keyboard is dismissed, and the bar would sit in edit mode with no
    // keyboard to type on.
    Connections {
        target: Qt.inputMethod
        onVisibleChanged: navigationBar.keyboardVisibilityChanged(Qt.inputMethod.visible)
    }

    // Every press on the bar, so a drag is seen from the start. It stays live while
    // the address is being edited -- the field is drawn above it and takes its own
    // presses, and everything else on the bar goes on working.
    //
    // The handler reaches above the bar as well. The drag that opens the grid has to
    // start somewhere the system's own bottom-edge swipe has not already taken, and
    // the bar alone lies in that strip; the reach gives a thumb somewhere higher to
    // start from. A tap up there does nothing -- the page does not get it either,
    // which is the price of the reach and the reason it is only a strip.
    MouseArea {
        id: gestureArea

        // Where the press went down, in the window's own coordinates. Not the bar's:
        // the bar rides on the deck, so while the deck follows the finger the bar
        // moves under it, and a distance measured against the bar would shrink as the
        // deck rose, drop the deck back, grow again -- which on device was the whole
        // screen jumping up and down for as long as the finger was held.
        property real pressedY: 0
        property real distance: 0
        property bool dragging: false
        property bool pressedOnBar: false
        property string pressedRegion: ""
        readonly property real reach: Theme.itemSizeExtraSmall

        objectName: "navigationBarGesture"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: -reach
            bottom: parent.bottom
        }

        function sceneY(y) {
            return gestureArea.mapToItem(null, 0, y).y
        }

        onPressed: {
            pressedY = sceneY(mouse.y)
            distance = 0
            dragging = false
            pressedOnBar = mouse.y >= reach
            pressedRegion = pressedOnBar ? navigationBar.regionAt(mouse.x) : ""
        }
        onPositionChanged: {
            distance = pressedY - sceneY(mouse.y)
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
            if (!dragging && pressedOnBar) {
                navigationBar.activate(navigationBar.regionAt(mouse.x))
            }
        }
    }

    // The bar is a pulley, so it says so -- along its top edge rather than its bottom.
    // The bottom edge is where the system watches for its own swipe, and an indicator
    // there invites a thumb to start the drag in exactly the wrong place.
    PullIndicator {
        objectName: "barPullIndicator"
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: Theme.paddingSmall
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
