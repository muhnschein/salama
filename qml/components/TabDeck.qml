// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The deck the browsing page is the top half of: two screens tall, browsing above and
// the tab grid below. Dragging the navigation bar upwards raises it and brings the grid
// up from under the page; dragging the grid past its top lowers it again. Nothing is
// pushed onto the page stack, so there is no sideways transition, and nothing to come
// back from (docs/DECISIONS/0010-tab-grid-deck.md).
//
// The deck holds where it is and how it gets there, and nothing of what it carries. The
// two layers are slots the browsing page fills, so what goes into them is still
// declared in BrowserPage.qml, in its context, with the engine that file alone imports.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: deck

    // The height of the page the deck is in, which Silica shrinks while the keyboard
    // is up.
    property real pageHeight: 0

    // Where the deck is headed and where a finger is holding it: tabsOpen is the
    // settled answer and changes the moment a gesture commits, tabsOffset is the
    // picture and takes the spring below to get there.
    property bool tabsOpen: false
    property bool dragging: false
    property real dragOffset: 0
    property real tabsOffset: dragging ? dragOffset : (tabsOpen ? fullHeight : 0)

    // The tallest the page has been. Resizing the engine's view mid-animation left the
    // content stretched until it finished; the deck keeps its height and lets the
    // keyboard cover it instead.
    property real fullHeight: 0

    // How far the deck must be dragged for the gesture to commit when the finger lifts.
    // Short, because the movement has already shown what letting go will do.
    readonly property real pullThreshold: Theme.itemSizeLarge

    // The browsing layer, and the grid's, below it.
    default property alias content: browserLayer.data
    property alias grid: gridSlot.data

    height: fullHeight * 2
    y: -tabsOffset

    function measure() {
        if (pageHeight > fullHeight) {
            fullHeight = pageHeight
        }
    }

    onPageHeightChanged: measure()
    Component.onCompleted: measure()

    // Enabled and disabled from the functions below rather than by a binding: the drag
    // ends by changing what tabsOffset is bound to, and two bindings on one property
    // are not ordered against each other -- the spring has to be on before it moves.
    Behavior on tabsOffset {
        id: deckSpring

        NumberAnimation {
            duration: 250
            easing.type: Easing.OutQuad
        }
    }

    // A finger takes the deck off whatever the spring was doing with it. Disabling the
    // Behavior does not stop an animation already under way, but the next value
    // written through it does -- the switch to dragOffset below. The animation cannot
    // be stopped by hand: it belongs to the Behavior, and Qt logs a warning and
    // ignores the call.
    function beginDrag() {
        deckSpring.enabled = false
        dragOffset = tabsOffset
        dragging = true
    }

    function dragTo(offset) {
        dragOffset = Math.max(0, Math.min(fullHeight, offset))
    }

    // A gesture that has ended: the deck goes all the way, one way or the other.
    function settle(open) {
        deckSpring.enabled = true
        tabsOpen = open
        dragging = false
    }

    Item {
        id: browserLayer

        objectName: "browserLayer"
        width: parent.width
        height: deck.fullHeight
    }

    Item {
        id: gridSlot

        objectName: "gridSlot"
        width: parent.width
        height: deck.fullHeight
        y: deck.fullHeight
    }
}
