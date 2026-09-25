// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Every press on the navigation bar, so a drag is seen from the start
// (docs/DECISIONS/0009-navigation-bar-gesture.md). It stays live while the address is
// being edited: the field is drawn above it and takes its own presses.
//
// It reaches above the bar as well, because the drag that opens the grid has to start
// where the system's own bottom-edge swipe has not. That reach lies over the foot of the
// page, where a player keeps its controls, so what it takes there that is not the drag
// -- a tap, a drag sideways or down -- it hands on to the page. A press that stays
// down is kept: the handle sits in the reach, and a finger resting on it before it
// moves up is the grid's.
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: gestureArea

    // The bar this is the gesture of: what a region means, and the signals to raise.
    property Item bar

    // Where the press went down, in the window's own coordinates: the bar rides on
    // the deck, so a distance measured against it shrank as the deck rose and grew
    // again, which on device was the screen jumping up and down under the finger.
    property real pressedY: 0
    property point pressedAt
    property point lastAt
    property real distance: 0
    property bool dragging: false
    property bool pressedOnBar: false
    // A press in the reach has been handed on to the page, and so is everything until
    // the finger lifts.
    property bool forwarding: false
    // Held too long to be a tap, and so not the page's.
    property bool heldDown: false
    property string pressedRegion: ""
    // Whether it reaches above the bar at all. Not while the omnibar's pane lies there
    // (docs/DECISIONS/0027-omnibar.md): a press on the pane is the pane's, and handed on
    // to the page from here it would reach a page the pane covers. The grid is still
    // pulled up from the bar itself.
    property bool reaching: true
    readonly property real reach: reaching ? Theme.itemSizeExtraSmall * 0.75 : 0
    // The whole bar, whole or slim: the page ends above it, so nothing under it is
    // the page's to take.
    readonly property real strip: bar ? bar.height : 0

    height: strip + reach

    function sceneY(y) {
        return gestureArea.mapToItem(null, 0, y).y
    }

    function scenePoint(mouse) {
        return gestureArea.mapToItem(null, mouse.x, mouse.y)
    }

    // Whether a finger that went down in the reach and has moved from there to here is
    // the page's: moved further than a shaky tap, and not mostly upwards -- upwards is
    // the drag that opens the grid.
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
    }
    // Only in the reach. On the bar a slow tap is still a tap: the event is handed
    // back, and MouseArea then raises clicked on release as though never held.
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
            bar.pageTouchStarted(pressedAt)
            bar.pageTouchMoved(lastAt)
            return
        }
        distance = pressedY - lastAt.y
        // Theme.startDragDistance is the movement Silica treats as a drag rather than a
        // shaky tap; past it the press belongs to the page, not a control.
        if (!dragging && distance > Theme.startDragDistance) {
            dragging = true
            pressedRegion = ""
            bar.dragStarted()
        }
        if (dragging) {
            bar.dragMoved(distance)
        }
    }
    onReleased: {
        var at = scenePoint(mouse)
        if (forwarding) {
            bar.pageTouchEnded(at)
        } else if (!dragging && !pressedOnBar && !heldDown) {
            // A tap in the reach: the page's, down and up where the finger was.
            bar.pageTouchStarted(pressedAt)
            bar.pageTouchEnded(at)
        }
        if (dragging) {
            bar.dragFinished(distance)
        }
        forwarding = false
        pressedRegion = ""
    }
    // The grab can be taken away mid-drag -- by the system's edge gesture, most of all.
    // Finish at zero so the page springs back rather than hanging, and end a touch the
    // page has where it was last.
    onCanceled: {
        if (dragging) {
            bar.dragFinished(0)
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
