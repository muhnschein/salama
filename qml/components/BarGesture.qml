// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Reaches above bar: grid drag must start clear of system bottom-edge swipe. Reach covers
// page foot (player controls): taps and sideways/down drags there forwarded to page; held
// press kept for handle.
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: gestureArea

    property Item bar

    // Page coords: bar rides deck, so bar-relative distance moves with deck -> jitter; window
    // axes turn with landscape page (Silica rotates page, window stays portrait).
    property real pressedY: 0
    property point pressedAt
    property point lastAt
    // From catch point, not press: else deck leaps on first frame.
    property real caughtY: 0
    property real distance: 0
    property bool dragging: false
    property bool pressedOnBar: false
    property bool forwarding: false
    property bool heldDown: false
    property string pressedRegion: ""
    // Off under omnibar pane: forwarding would hit covered page.
    property bool reaching: true
    readonly property real reach: reaching ? Theme.itemSizeExtraSmall * 0.75 : 0
    readonly property real strip: bar ? bar.height : 0

    height: strip + reach

    // Nearest page: still while deck moves, upright in either orientation.
    function frame() {
        var item = gestureArea.parent
        while (item && item.allowedOrientations === undefined) {
            item = item.parent
        }
        return item
    }

    function sceneY(y) {
        return gestureArea.mapToItem(frame(), 0, y).y
    }

    function scenePoint(mouse) {
        return gestureArea.mapToItem(frame(), mouse.x, mouse.y)
    }

    // Reach press is page's if past shaky-tap distance and not mostly up (up = grid drag).
    function isPageMove(from, to) {
        var acrossX = to.x - from.x
        var acrossY = to.y - from.y
        if (Math.max(Math.abs(acrossX), Math.abs(acrossY)) <= Theme.startDragDistance) {
            return false
        }
        return !(acrossY < 0 && -acrossY >= Math.abs(acrossX))
    }

    onPressed: {
        pressedY = sceneY(mouse.y)
        pressedAt = scenePoint(mouse)
        lastAt = pressedAt
        distance = 0
        dragging = false
        forwarding = false
        heldDown = false
        pressedOnBar = mouse.y >= reach
        pressedRegion = pressedOnBar ? bar.regionAt(mouse.x) : ""
        // Arm on press so grid's first-show cost paid before motion, not in first drag frame.
        bar.dragArmed()
    }
    // Reach only. On bar slow tap still tap: rejecting makes MouseArea emit clicked on release.
    onPressAndHold: {
        if (pressedOnBar) {
            mouse.accepted = false
        } else {
            heldDown = true
        }
    }
    onPositionChanged: {
        lastAt = scenePoint(mouse)
        if (forwarding) {
            bar.pageTouchMoved(lastAt)
            return
        }
        if (!dragging && !pressedOnBar && !heldDown && isPageMove(pressedAt, lastAt)) {
            forwarding = true
            bar.dragDisarmed()
            bar.pageTouchStarted(pressedAt)
            bar.pageTouchMoved(lastAt)
            return
        }
        if (!dragging && pressedY - lastAt.y > Theme.startDragDistance) {
            dragging = true
            caughtY = lastAt.y
            pressedRegion = ""
            bar.dragStarted()
        }
        if (dragging) {
            distance = caughtY - lastAt.y
            bar.dragMoved(distance)
        }
    }
    onReleased: {
        var at = scenePoint(mouse)
        if (forwarding) {
            bar.pageTouchEnded(at)
        } else if (!dragging && !pressedOnBar && !heldDown) {
            bar.pageTouchStarted(pressedAt)
            bar.pageTouchEnded(at)
        }
        if (dragging) {
            bar.dragFinished(distance)
        } else if (!forwarding) {
            bar.dragDisarmed()
        }
        forwarding = false
        pressedRegion = ""
    }
    // Grab stolen (system edge gesture): finish at 0 so page springs back.
    onCanceled: {
        if (dragging) {
            bar.dragFinished(0)
        } else if (!forwarding) {
            bar.dragDisarmed()
        }
        if (forwarding) {
            bar.pageTouchEnded(lastAt)
        }
        dragging = false
        forwarding = false
        pressedRegion = ""
    }
    onClicked: {
        if (!dragging && pressedOnBar) {
            bar.activate(bar.regionAt(mouse.x))
        }
    }
}
