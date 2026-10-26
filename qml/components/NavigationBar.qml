// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// One MouseArea (BarGesture) owns every press: handler behind controls never reached, one
// letting presses through can't see later movement. Drag reported as distance: gesture
// showing nothing until threshold looks on device like system edge swipe stole touch.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: navigationBar

    property Item view: null
    property string url: TabModel.activeUrl
    readonly property bool loading: view ? view.loading === true : false
    property bool canGoBack: view ? view.canGoBack === true : false
    // Back hold lists tab history; false -> hold = slow tap.
    property bool holdsBack: false
    readonly property int loadProgress: view ? view.loadProgress : 0
    // Gecko verdict, https only: validState = has one, allGood covers cert, protocol, mixed
    // content. Not for reader view: app's own document, no connection.
    readonly property bool tlsBroken: {
        if (!view || url.indexOf("https://") !== 0 || (view.reader && view.reader.active)) {
            return false
        }
        var security = view.security
        return !!security && !!security.validState && !security.allGood
    }
    readonly property int mediaState: TabModel.activeMediaState
    readonly property bool muted: TabModel.activeMuted
    property bool editing: false
    readonly property string typedText: urlField.text
    property string openedWith
    readonly property bool edited: typedText !== openedWith
    property bool forNewTab: false
    readonly property bool paneUp: editing && (forNewTab
                                               || (edited && typedText.trim().length > 0))
    // Slim instead of hiding on scroll. Tap restores whole bar; only tap on whole bar edits.
    property bool compact: false
    // Landscape: every control at slim height, bar stays put.
    property bool landscape: false

    signal accepted(string text, bool inNewTab)
    signal back()
    // Press, before hold or tap: time to fetch what hold shows.
    signal backPressed()
    signal backHeld()
    signal reloadOrStop()
    signal showMenu()
    // Upward px from catch point, negative below.
    signal dragArmed()
    signal dragDisarmed()
    signal dragStarted()
    signal dragMoved(real distance)
    signal dragFinished(real distance)
    // Reach touch that turned out page's. Page coords.
    signal pageTouchStarted(point position)
    signal pageTouchMoved(point position)
    signal pageTouchEnded(point position)

    readonly property real addressLeft: compact ? Theme.horizontalPageMargin
                                                : backIcon.x + backIcon.width + Theme.paddingMedium
    readonly property real addressRight: compact ? width - Theme.horizontalPageMargin
                                                 : reloadIcon.x - Theme.paddingMedium
    readonly property real centredWidth: 2 * Math.min(width / 2 - addressLeft,
                                                      addressRight - width / 2)

    // Edit slab from bar's edge to menu; text inside starts at page margin, under back icon.
    readonly property real fieldLeft: 0
    readonly property real fieldEnd: menuIcon.x - Theme.paddingLarge
    // Omnibar's landscape go/search pills sit between field and menu.
    property real pillsWidth: 0
    readonly property real fieldRight: fieldEnd - (pillsWidth > 0 ? pillsWidth + Theme.paddingMedium
                                                                  : 0)

    // Bar sits in system edge-swipe strip; its height is gesture start area, so slim gives
    // up only a quarter.
    readonly property real slimHeight: Theme.itemSizeSmall
    // 0 slim .. 1 whole. All state differences derive from this so height animation
    // carries everything.
    readonly property real expansion: landscape ? 1.0 : (height - slimHeight)
                                                      / (Theme.itemSizeLarge - slimHeight)
    readonly property bool resizing: heightSlide.running

    height: compact || landscape ? slimHeight : Theme.itemSizeLarge

    Behavior on height {
        NumberAnimation {
            id: heightSlide

            duration: 200
            easing.type: Easing.InOutQuad
        }
    }

    function beginEditing(newTab) {
        forNewTab = newTab === true
        openedWith = forNewTab ? "" : navigationBar.url
        urlField.text = openedWith
        editing = true
        urlField.forceActiveFocus()
        if (!forNewTab) {
            urlField.selectAll()
        }
    }

    function endEditing() {
        if (!editing) {
            return
        }
        editing = false
        forNewTab = false
        urlField.focus = false
    }

    // Focus loss or keyboard hide ends editing, else bar stuck in edit mode. Not while pane
    // up: field drops focus with keyboard so tap brings both back.
    function focusChanged(hasFocus) {
        if (!hasFocus && !paneUp) {
            endEditing()
        }
    }

    function keyboardVisibilityChanged(keyboardVisible) {
        if (!keyboardVisible && paneUp) {
            urlField.focus = false
        } else if (!keyboardVisible) {
            endEditing()
        }
    }

    function submit() {
        var text = urlField.text
        var inNewTab = forNewTab
        endEditing()
        if (text.length > 0) {
            navigationBar.accepted(text, inNewTab)
        }
    }

    // Named regions, not hit testing: handler sits on top, childAt() returns handler.
    function regionAt(x) {
        if (navigationBar.compact) {
            return "address"
        }
        if (x >= menuIcon.x) {
            return "menu"
        }
        if (!navigationBar.editing) {
            if (x < navigationBar.addressLeft) {
                return "back"
            }
            if (x >= navigationBar.addressRight) {
                return "reload"
            }
            if (addressRow.showsMute && x - addressRow.x < addressRow.muteEnd) {
                return "mute"
            }
        }
        return "address"
    }

    function activate(region) {
        if (region === "menu") {
            // Keyboard would cover menu.
            navigationBar.endEditing()
            navigationBar.showMenu()
        } else if (region === "back") {
            if (navigationBar.canGoBack) {
                navigationBar.back()
            }
        } else if (region === "reload") {
            navigationBar.reloadOrStop()
        } else if (region === "mute") {
            PageMedia.toggleMuted(TabModel.activeTabId)
        } else if (region === "address") {
            navigationBar.tapAddress()
        }
    }

    function regionPressed(region) {
        if (region === "back" && navigationBar.canGoBack) {
            navigationBar.backPressed()
        }
    }

    // True = hold handled, no tap follows.
    function hold(region) {
        if (region === "back" && navigationBar.canGoBack && navigationBar.holdsBack) {
            navigationBar.backHeld()
            return true
        }
        return false
    }

    // Restore via engine chrome state, which slims again on scroll.
    function tapAddress() {
        if (navigationBar.compact) {
            if (navigationBar.view) {
                navigationBar.view.chrome = true
            }
        } else if (!navigationBar.editing) {
            navigationBar.beginEditing()
        }
    }

    // Opaque: faded host unreadable over light page.
    Rectangle {
        objectName: "navigationBarBackground"
        anchors.fill: parent
        color: Theme.highlightDimmerColor
    }

    // sailfish-browser's ProgressBar: highlight wash fills whole bar behind controls, fading
    // downward, fades out when load ends.
    Item {
        objectName: "loadProgress"
        anchors.fill: parent
        opacity: navigationBar.loading ? 1.0 : 0.0
        visible: navigationBar.loading || opacity > 0

        Behavior on opacity {
            FadeAnimation {}
        }

        Rectangle {
            objectName: "loadProgressFill"
            height: parent.height
            width: parent.width * navigationBar.loadProgress / 100
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: Theme.rgba(Theme.highlightBackgroundColor, Theme.opacityHigh)
                }
                GradientStop {
                    position: 1.0
                    color: Theme.rgba(Theme.highlightBackgroundColor, 0.0)
                }
            }

            Behavior on width {
                enabled: parent.opacity === 1.0
                SmoothedAnimation {
                    velocity: 480
                    duration: 200
                }
            }
        }
    }

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
        opacity: navigationBar.expansion * (navigationBar.view ? 1.0 : Theme.opacityLow)
        visible: !navigationBar.editing && opacity > 0
        source: navigationBar.loading ? "image://theme/icon-m-reset"
                                      : "image://theme/icon-m-refresh"
        highlighted: gestureArea.pressedRegion === "reload"
    }

    AddressLabel {
        id: addressRow

        objectName: "addressRow"
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        visible: !navigationBar.editing
        z: 1
        url: navigationBar.url
        tlsBroken: navigationBar.tlsBroken
        pressed: gestureArea.pressedRegion === "address"
        mediaState: navigationBar.mediaState
        muted: navigationBar.muted
        mutePressed: gestureArea.pressedRegion === "mute"
        maximumWidth: navigationBar.centredWidth
        fontSize: Theme.fontSizeSmall
                  + (Theme.fontSizeMedium - Theme.fontSizeSmall) * navigationBar.expansion
        iconSize: Theme.iconSizeSmall
                  + (Theme.iconSizeSmallPlus - Theme.iconSizeSmall) * navigationBar.expansion
        sliding: navigationBar.resizing
    }

    AddressField {
        id: urlField

        x: navigationBar.fieldLeft
        width: navigationBar.fieldRight - navigationBar.fieldLeft
        height: parent.height
        // Above gesture handler: field needs own presses for caret.
        z: 1
        visible: navigationBar.editing
        keepsFocus: navigationBar.paneUp
        EnterKey.onClicked: navigationBar.submit()
        onActiveFocusChanged: navigationBar.focusChanged(activeFocus)
    }

    // Field can keep focus after keyboard dismissed; would leave bar in edit mode.
    Connections {
        target: Qt.inputMethod
        onVisibleChanged: navigationBar.keyboardVisibilityChanged(Qt.inputMethod.visible)
    }

    // Covers reach above bar so drag seen from start; not while pane up.
    BarGesture {
        id: gestureArea

        objectName: "navigationBarGesture"
        bar: navigationBar
        reaching: !navigationBar.paneUp
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            topMargin: -reach
        }
    }
}
