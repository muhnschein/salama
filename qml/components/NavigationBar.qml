// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The bar along the bottom of the browsing page: back, the address, reload/stop and
// the menu. Dragging it upwards pulls the tab grid up from underneath the page.
//
// While the address is being edited the bar belongs to the field: back and reload are
// not drawn and the field takes their room, from the edge of the screen to the menu
// (docs/DECISIONS/0009-navigation-bar-gesture.md).
//
// One MouseArea covers the whole bar and owns every press, and the icons are just
// icons: a handler behind the controls is never reached, while one that lets presses
// through to them cannot see the movement afterwards. So the press is taken here and
// the region under it decides what a tap means and what is drawn pressed.
//
// The drag is reported as a distance, not as a finished gesture: the page follows the
// finger while it moves and decides when it lifts. A gesture that shows nothing until
// its threshold is, on device, the system's own edge swipe having taken the touch.
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
    // certificate, a broken chain, mixed content.
    property bool tlsBroken: false
    // The address turns into a field in place while it is being edited.
    property bool editing: false
    // Slimmed down to the handle and the host, with the controls off it: what the bar
    // does instead of leaving when a page is scrolled
    // (docs/DECISIONS/0009-navigation-bar-gesture.md).
    property bool compact: false

    signal accepted(string text)
    signal back()
    signal reloadOrStop()
    signal showMenu()
    // Upward drag, in pixels from where the finger went down. Negative means it has
    // come back below its own starting point.
    signal dragStarted()
    signal dragMoved(real distance)
    signal dragFinished(real distance)

    // Where the address may be drawn: between the two controls that flank it, or the
    // whole bar but its margins when there are no controls on it.
    readonly property real addressLeft: compact ? Theme.horizontalPageMargin
                                                : backIcon.x + backIcon.width + Theme.paddingMedium
    readonly property real addressRight: compact ? width - Theme.horizontalPageMargin
                                                 : reloadIcon.x - Theme.paddingMedium
    // The widest the address can be while staying centred on the screen rather than
    // in the space left over between the controls.
    readonly property real centredWidth: 2 * Math.min(width / 2 - addressLeft,
                                                      addressRight - width / 2)

    // Where the field is drawn instead: everything the menu does not take. Back and
    // reload are gone while it is up, so their room is the field's, and the margin
    // left is the smallest one that still keeps text off the edge of the screen.
    readonly property real fieldLeft: Theme.paddingMedium
    readonly property real fieldRight: menuIcon.x - Theme.paddingMedium

    // The bar is the whole touch target for the drag, and it sits in the strip the
    // system watches for its own edge swipe. Every bit of height here is height the
    // gesture can start in without lipstick taking it first -- which is why the slim
    // state gives up a quarter of it and not more.
    height: compact ? Theme.itemSizeSmall : Theme.itemSizeLarge

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
    // the page while the field was up used to leave the bar in edit mode with nothing
    // to type into. Named functions, so the load tests can exercise both.
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

    // Silica lays a field out with room for its label and underline, so centring the
    // item leaves the text off centre; the field publishes the offset for this.
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
    // testing: the handler sits on top of everything, so childAt() would only ever
    // return the handler. They tile the bar, so each target is larger than its icon.
    function regionAt(x) {
        // Nothing is on the slim bar but the address.
        if (navigationBar.compact) {
            return "address"
        }
        if (x >= menuIcon.x) {
            return "menu"
        }
        // While the field is up, the room those two had is the field's.
        if (!navigationBar.editing) {
            if (x < navigationBar.addressLeft) {
                return "back"
            }
            if (x >= navigationBar.addressRight) {
                return "reload"
            }
        }
        return "address"
    }

    function activate(region) {
        if (region === "menu") {
            navigationBar.showMenu()
        } else if (region === "back") {
            if (navigationBar.canGoBack) {
                navigationBar.back()
            }
        } else if (region === "reload") {
            navigationBar.reloadOrStop()
        } else if (region === "address") {
            if (!navigationBar.editing) {
                navigationBar.beginEditing()
            }
        }
    }

    // Opaque, and the page ends above it rather than running underneath: a
    // translucent bar looks better than it works, and the last rows of a page kept
    // being unreachable behind it (docs/DECISIONS/0009-navigation-bar-gesture.md).
    Rectangle {
        objectName: "navigationBarBackground"
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    // Where the drag starts, drawn: on the line between the bar and the page, which
    // is where the finger is aiming, and inside the reach the handler covers above
    // the bar.
    DragHandle {
        objectName: "barDragHandle"
        x: (navigationBar.width - width) / 2
        y: -height / 2
        active: gestureArea.pressed || gestureArea.dragging
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
        visible: !navigationBar.editing && !navigationBar.compact
        source: "image://theme/icon-m-back"
        opacity: navigationBar.canGoBack ? 1.0 : Theme.opacityLow
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
        visible: !navigationBar.compact
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
        visible: !navigationBar.editing && !navigationBar.compact
        source: navigationBar.loading ? "image://theme/icon-m-clear"
                                      : "image://theme/icon-m-refresh"
        highlighted: gestureArea.pressedRegion === "reload"
    }

    // Centred on the screen rather than in the space between the controls: an address
    // that sits off to one side reads as a label rather than as the bar's subject.
    AddressLabel {
        objectName: "addressRow"
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        // While editing, the field below says everything this row would.
        visible: !navigationBar.editing
        // Above the gesture handler: nothing here accepts a press in any case.
        z: 1
        url: navigationBar.url
        tlsBroken: navigationBar.tlsBroken
        privateTab: navigationBar.privateTab
        pressed: gestureArea.pressedRegion === "address"
        maximumWidth: navigationBar.centredWidth
        fontSize: navigationBar.compact ? Theme.fontSizeSmall : Theme.fontSizeMedium
    }

    TextField {
        id: urlField

        objectName: "addressField"
        x: navigationBar.fieldLeft
        width: navigationBar.fieldRight - navigationBar.fieldLeft
        anchors {
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: navigationBar.textCentringOffset(urlField)
        }
        // Above the gesture handler: while the address is being edited the field needs
        // its own presses for the caret, and the rest of the bar stays live.
        z: 1
        visible: navigationBar.editing
        label: navigationBar.privateTab ? qsTr("Private tab") : ""
        placeholderText: qsTr("Search or enter address")
        // The same size the host is drawn at, so the text does not jump when the
        // label becomes a field.
        font.pixelSize: Theme.fontSizeMedium
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
        EnterKey.enabled: text.length > 0
        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.onClicked: navigationBar.submit()
        onActiveFocusChanged: navigationBar.focusChanged(activeFocus)
    }

    // Silica insets the text inside a field by a page margin at each end, which is a
    // page's margin, not a bar's. Through Binding rather than as properties: a Silica
    // without them should cost a line in the log rather than a bar that fails to load.
    Binding {
        target: urlField
        property: "textLeftMargin"
        value: Theme.paddingMedium
    }

    Binding {
        target: urlField
        property: "textRightMargin"
        value: Theme.paddingMedium
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
    // presses, and the rest of the bar goes on working.
    //
    // The handler reaches above the bar as well: the drag that opens the grid has to
    // start somewhere the system's own bottom-edge swipe has not already taken. A tap
    // up there does nothing, which is the price of the reach and why it is a strip.
    MouseArea {
        id: gestureArea

        // Where the press went down, in the window's own coordinates: the bar rides on
        // the deck, so a distance measured against it shrank as the deck rose and grew
        // again, which on device was the screen jumping up and down under the finger.
        property real pressedY: 0
        property real distance: 0
        property bool dragging: false
        property bool pressedOnBar: false
        property string pressedRegion: ""
        readonly property real reach: Theme.itemSizeExtraSmall * 0.75

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
