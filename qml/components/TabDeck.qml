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
    // Grid drawn: dragged or open, or drag may start (first drawn frame uploads all previews).
    readonly property bool gridShown: tabsOffset > 0 || primed

    // Max height seen in this orientation: resizing engine view mid-animation stretches
    // content; keyboard covers. Keyboard never makes portrait page wider than tall, so a
    // flip of that is a rotation.
    property real fullHeight: 0
    // Shape at last flip. Silica turns width before height: square halfway counts as tall,
    // either way round the max then settles it.
    property bool measuredTall: true

    readonly property real pullThreshold: Theme.itemSizeLarge

    default property alias content: browserLayer.data
    property alias grid: gridSlot.data

    height: fullHeight * 2
    y: -tabsOffset

    function measure() {
        var tall = pageHeight >= width
        var height = tall === measuredTall ? Math.max(fullHeight, pageHeight) : pageHeight
        measuredTall = tall
        if (height !== fullHeight) {
            // Rotated, or second side of turn: jump, no spring sliding open grid across.
            var spring = deckSpring.enabled
            deckSpring.enabled = false
            fullHeight = height
            deckSpring.enabled = spring
        }
    }

    // Hidden grid takes turn's size once turn is over: laid out in turn's frame, it remade its
    // previews and held up engine's resize there. Shown, or about to be: at once.
    function placeGrid() {
        if (gridShown || gridSlot.width === 0 || gridSlot.height === 0) {
            sizeGrid()
        } else {
            gridCatchUp.restart()
        }
    }

    function sizeGrid() {
        gridCatchUp.stop()
        gridSlot.width = width
        gridSlot.height = fullHeight
    }

    onWidthChanged: {
        measure()
        placeGrid()
    }
    onPageHeightChanged: measure()
    onFullHeightChanged: placeGrid()
    onGridShownChanged: {
        if (gridShown && gridCatchUp.running) {
            sizeGrid()
        }
    }
    Component.onCompleted: measure()

    Timer {
        id: gridCatchUp

        interval: 300
        onTriggered: deck.sizeGrid()
    }

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
        y: deck.fullHeight
    }
}
