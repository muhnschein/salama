// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The bar along the bottom of the browsing page: back, the address, reload/stop and
// the menu. Dragging it upwards pulls the tab grid up from underneath the page.
//
// While the address is being edited the bar belongs to the field: back and reload are
// not drawn and the field takes their room, from the edge of the screen to the menu
// (docs/DECISIONS/0009-navigation-bar-gesture.md).
//
// One MouseArea owns every press and the icons are just icons: a handler behind the
// controls is never reached, while one that lets presses through cannot see the
// movement afterwards. So the press is taken by BarGesture and the region under it
// decides what a tap means. The drag is reported as a distance rather than as a
// finished gesture, because a gesture that shows nothing until its threshold is, on
// device, indistinguishable from the system's own edge swipe having taken the touch.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: navigationBar

    // The page in front, or null while it is made.
    property Item view: null
    property string url
    property bool loading: false
    property bool canGoBack: false
    readonly property int loadProgress: view ? view.loadProgress : 0
    // The page came over TLS and the engine is not satisfied with it: a bad
    // certificate, a broken chain, mixed content. Gecko's own verdict, if this engine
    // build hands one out: validState says it has one for this page, allGood weighs
    // certificate, protocol and mixed content. sailfish-browser reads the same two,
    // and only for https. Not for a reader view, whose address is the article's but
    // whose document is one of this application's, and came over no connection at all
    // (docs/DECISIONS/0023-reader-view.md).
    readonly property bool tlsBroken: {
        if (!view || url.indexOf("https://") !== 0 || (view.reader && view.reader.active)) {
            return false
        }
        var security = view.security
        return !!security && !!security.validState && !security.allGood
    }
    // The address turns into a field in place while it is being edited.
    property bool editing: false
    // Slimmed down to the handle and the host, with the controls faded off it: what the
    // bar does instead of leaving when a page is scrolled (docs/DECISIONS/0009). A tap
    // on it brings the whole bar back, and only a tap on the whole bar edits.
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
    // A touch in the reach above the bar that turned out to be the page's: a tap, or a
    // drag any way but up. Points are in the window's coordinates; the page hands them
    // to the engine.
    signal pageTouchStarted(point position)
    signal pageTouchMoved(point position)
    signal pageTouchEnded(point position)

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
    // system watches for its own edge swipe: every bit of height here is height the
    // gesture can start in, which is why the slim state gives up only a quarter. The
    // page ends above the bar in either state.
    readonly property real slimHeight: Theme.itemSizeSmall
    // 0 while slim and 1 while whole. Everything that differs between the two states
    // is drawn from this, so the height animation below carries all of it and nothing
    // needs an animation of its own.
    readonly property real expansion: (height - slimHeight) / (Theme.itemSizeLarge - slimHeight)
    readonly property bool resizing: heightSlide.running

    height: compact ? slimHeight : Theme.itemSizeLarge

    Behavior on height {
        NumberAnimation {
            id: heightSlide

            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

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
            // The menu comes up from under the bar, where the keyboard would cover it.
            navigationBar.endEditing()
            navigationBar.showMenu()
        } else if (region === "back") {
            if (navigationBar.canGoBack) {
                navigationBar.back()
            }
        } else if (region === "reload") {
            navigationBar.reloadOrStop()
        } else if (region === "address") {
            navigationBar.tapAddress()
        }
    }

    // A tap on the slim bar brings back the whole bar, controls and all, and the next
    // tap edits. It is brought back the way scrolling a page back up brings it back:
    // through the engine's chrome state, which slims it again as the page is scrolled
    // on down.
    function tapAddress() {
        if (navigationBar.compact) {
            if (navigationBar.view) {
                navigationBar.view.chrome = true
            }
        } else if (!navigationBar.editing) {
            navigationBar.beginEditing()
        }
    }

    // Opaque, whole or slim. It faded away as it slimmed once, and on device the host
    // was unreadable over a light page; the page ends above the bar instead, so it
    // hides nothing (docs/DECISIONS/0009).
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
        active: gestureArea.pressed && !gestureArea.forwarding || gestureArea.dragging
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
        // Faded with the bar, and dimmed on top of that when there is nowhere to go.
        opacity: navigationBar.expansion * (navigationBar.canGoBack ? 1.0 : Theme.opacityLow)
        visible: !navigationBar.editing && navigationBar.expansion > 0
        source: "image://theme/icon-m-back"
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
        opacity: navigationBar.expansion
        visible: opacity > 0
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
        opacity: navigationBar.expansion
        visible: !navigationBar.editing && opacity > 0
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
        pressed: gestureArea.pressedRegion === "address"
        maximumWidth: navigationBar.centredWidth
        fontSize: Theme.fontSizeSmall
                  + (Theme.fontSizeMedium - Theme.fontSizeSmall) * navigationBar.expansion
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
    // page's margin, not a bar's. Through Binding: a Silica without them should cost a
    // line in the log rather than a bar that fails to load.
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

    // Every press on the bar and just above it, so a drag is seen from the start.
    BarGesture {
        id: gestureArea

        objectName: "navigationBarGesture"
        bar: navigationBar
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: -reach
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
