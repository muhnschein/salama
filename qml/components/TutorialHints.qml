// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Where the tutorial shows each step, and how: Silica's TapInteractionHint for a tap and
// TouchInteractionHint for a movement, placed over the sketch the step is about
// (docs/DECISIONS/0034-tutorial.md). A tap on the address, on the menu button, or beside
// the menu; up from the bar's handle, where the browsing page's drag is aimed; across the
// first row of cells, left to close a tab and right to carry one on; down onto the other
// group's name; and down from the upper third, where Silica starts a pull.
//
// Laid over the whole page, since both hints place themselves by their parent's size.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: hints

    // The sketch's parts the hints are placed over.
    property Item bar
    property Item grid
    property Item menu

    // Where a point of the sketch will be once the deck is at rest: in the coordinates of
    // the layer it is on, which lies over the whole page when that layer is the one shown.
    // Not where it is now, since a step changes the moment a finger lifts, while the
    // deck is still springing into place.
    function placeOf(item, point, layer) {
        return item.mapToItem(layer, point.x, point.y)
    }

    function stop() {
        tapHint.stop()
        touchHint.stop()
    }

    // The hint for a step, which the page asks for once the step waits for its gesture.
    function show(step) {
        stop()
        if (step === "address" || step === "menu" || step === "menuOpen") {
            var target = Qt.point(width / 2, (height - menu.sheetHeight) / 2)
            if (step === "address") {
                target = placeOf(bar, bar.addressCentre, bar.parent)
            } else if (step === "menu") {
                target = placeOf(bar, bar.menuCentre, bar.parent)
            }
            tapHint.x = target.x - tapHint.width / 2
            tapHint.y = target.y - tapHint.height / 2
            tapHint.restart()
            return
        }
        var firstRow = grid.cellCentre(0)
        var group = placeOf(grid.strip, grid.strip.otherCentre, grid)
        touchHint.anchors.horizontalCenterOffset = 0
        touchHint.anchors.verticalCenterOffset = 0
        touchHint.interactionMode = TouchInteraction.Swipe
        if (step === "open") {
            touchHint.interactionMode = TouchInteraction.Pull
            touchHint.direction = TouchInteraction.Up
            touchHint.startY = height - bar.height - touchHint.height / 2
        } else if (step === "closeTab") {
            touchHint.direction = TouchInteraction.Left
            touchHint.startX = width * 7 / 8 - touchHint.width / 2
            touchHint.anchors.verticalCenterOffset = firstRow.y - height / 2
        } else if (step === "moveTab") {
            touchHint.direction = TouchInteraction.Right
            touchHint.startX = firstRow.x - touchHint.width / 2
            touchHint.anchors.verticalCenterOffset = firstRow.y - height / 2
        } else if (step === "groupTab") {
            touchHint.direction = TouchInteraction.Down
            touchHint.startY = group.y - touchHint.distance - touchHint.height / 2
            touchHint.anchors.horizontalCenterOffset = group.x - width / 2
        } else {
            touchHint.interactionMode = TouchInteraction.Pull
            touchHint.direction = TouchInteraction.Down
            touchHint.startY = height / 3 - touchHint.height / 2
        }
        touchHint.restart()
    }

    TapInteractionHint {
        id: tapHint

        objectName: "tutorialTapHint"
        running: false
    }

    TouchInteractionHint {
        id: touchHint

        objectName: "tutorialTouchHint"
        loops: Animation.Infinite
    }
}
