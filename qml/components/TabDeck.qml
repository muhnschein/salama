// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Two screens tall: browsing above, tab grid below. Layers are slots BrowserPage.qml fills
// (only file importing engine).
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: deck

    // Silica shrinks it while keyboard up.
    property real pageHeight: 0

    // tabsOpen = settled target; tabsOffset = drawn position, springs there.
    property bool tabsOpen: false
    property bool dragging: false
    property real dragOffset: 0
    property real tabsOffset: dragging ? dragOffset : (tabsOpen ? fullHeight : 0)
    property bool primed: false

    // Max height seen: resizing engine view mid-animation stretches content; keyboard covers.
    property real fullHeight: 0

    readonly property real pullThreshold: Theme.itemSizeLarge

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

    // Toggled in functions, not binding: two bindings aren't ordered, and spring must be
    // on before tabsOffset rebinds.
    Behavior on tabsOffset {
        id: deckSpring

        NumberAnimation {
            duration: 250
            easing.type: Easing.OutQuad
        }
    }

    // Disabling Behavior doesn't stop running animation, but next written value does.
    // Can't stop by hand: Behavior owns it, Qt warns and ignores.
    function beginDrag() {
        deckSpring.enabled = false
        dragOffset = tabsOffset
        dragging = true
    }

    function prime() {
        primed = true
    }

    function unprime() {
        if (!dragging) {
            primed = false
        }
    }

    function dragTo(offset) {
        dragOffset = Math.max(0, Math.min(fullHeight, offset))
    }

    function settle(open) {
        deckSpring.enabled = true
        tabsOpen = open
        dragging = false
        primed = false
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
