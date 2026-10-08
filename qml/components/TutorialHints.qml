// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Covers page: Silica interaction hints size from parent.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: hints

    property Item bar
    property Item grid
    property Item menu

    // Position at deck rest: step changes on finger lift while deck still springs.
    function placeOf(item, point, layer) {
        return item.mapToItem(layer, point.x, point.y)
    }

    function stop() {
        tapHint.stop()
        touchHint.stop()
    }

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
