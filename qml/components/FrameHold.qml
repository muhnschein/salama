// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Engine draws its last frame stretched to the view's new size until it draws one at that size:
// after a turn, for a second or two on heavy pages (device). Turn: page colour over turned view
// (upright frame in it read as one more movement, device), held until engine reports a visible
// area of the view's own shape. Engine first lays page out at new size but old rotation (its
// rotation waits out a filter): that area is the view's shape on its side, still held. Bar
// changes no longer resize page.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: hold

    property Item view: null
    // Engine silent or slow: show what it has.
    property int patience: 1500

    property bool holding: false
    // Turn coming, resize to hold.
    property bool armed: false

    // Engine reports it every scroll frame: read only while holding, no JS per frame otherwise.
    readonly property rect area: holding && view && view.contentRect ? view.contentRect
                                                                     : Qt.rect(0, 0, 0, 0)
    // Finger on page: page must follow it. New document: nothing of the old to cover.
    readonly property bool interrupted: !!view && (view.moving === true || view.dragging === true
                                                   || view.pinching === true
                                                   || view.painted === false)

    function aspect(width, height) {
        return width > 0 && height > 0 ? width / height : 0
    }

    // Blank or hidden view stretches nothing.
    function usable() {
        return !!view && view.painted === true && view.visible && view.width > 0
                && view.height > 0
    }

    // Turn coming: page colour over view once it turns.
    function cover() {
        if (holding || armed || !usable()) {
            return
        }
        fade.stop()
        opacity = 0
        armed = true
        disarm.restart()
    }

    function viewResized() {
        if (holding) {
            settle.stop()
            checkArea()
        } else if (armed) {
            armed = false
            disarm.stop()
            holding = true
            fade.stop()
            opacity = 1
            patienceTimer.restart()
            checkArea()
        }
    }

    // Started once, not again on each later report: busy page reports every frame (device).
    function checkArea() {
        var shape = aspect(view ? view.width : 0, view ? view.height : 0)
        if (holding && !settle.running && shape > 0
                && Math.abs(aspect(area.width, area.height) - shape) <= shape / 100) {
            settle.start()
        }
    }

    function release() {
        if (!holding) {
            return
        }
        holding = false
        settle.stop()
        patienceTimer.stop()
        fade.restart()
    }

    // Other tab, finger on page, new document: gone at once, no fade over page that moves.
    function drop() {
        holding = false
        armed = false
        settle.stop()
        patienceTimer.stop()
        disarm.stop()
        fade.stop()
        opacity = 0
    }

    objectName: "frameHold"
    opacity: 0
    visible: opacity > 0 && !!view && view.visible

    onViewChanged: drop()
    onAreaChanged: checkArea()
    onInterruptedChanged: {
        if (interrupted) {
            drop()
        }
    }

    Connections {
        target: hold.view
        onWidthChanged: hold.viewResized()
        onHeightChanged: hold.viewResized()
    }

    // Engine draws new size a frame or two after it reports it.
    Timer {
        id: settle

        interval: 50
        onTriggered: hold.release()
    }

    Timer {
        id: patienceTimer

        interval: hold.patience
        onTriggered: hold.release()
    }

    // Turn that never came.
    Timer {
        id: disarm

        interval: 1000
        onTriggered: hold.armed = false
    }

    // Heavy page goes on redrawing a second or two after (device): slow fade lets it settle.
    NumberAnimation {
        id: fade

        objectName: "frameHoldFade"
        target: hold
        property: "opacity"
        to: 0
        duration: 600
    }

    Rectangle {
        objectName: "frameHoldFill"
        anchors.fill: parent
        color: hold.view && hold.view.backgroundColor ? hold.view.backgroundColor
                                                      : Theme.highlightDimmerColor
    }
}
