// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Engine draws its last frame stretched to the view's new size until it draws one at that
// size: after a turn, for a second or two on heavy pages (device). Bar's change: as Chrome on
// Android, frame taken before it stays unscaled at top left over page colour. Turn: page colour
// alone, upright frame in turned view read as one more movement (device). Held until engine
// lays page out at new size; it then reports a visible area of the new shape.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: hold

    property Item view: null
    // Engine silent or slow: show what it has.
    property int patience: 1500

    property bool holding: false
    // Frame taken, change to come.
    property bool armed: false
    property bool grabbing: false
    // Frame asked for; one answering an older ask (other tab since) is dropped.
    property int ticket: 0
    property var pendingApply: null
    property var shot: null
    property size frameSize: Qt.size(0, 0)
    // Shape of engine's visible area at last resize: a new one means engine drew new size.
    property real staleAspect: 0

    readonly property rect area: view && view.contentRect ? view.contentRect : Qt.rect(0, 0, 0, 0)
    // Finger on page: page must follow it. New document: frame isn't of it.
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
        if (grabbing || holding || armed || !usable()) {
            return
        }
        fade.stop()
        opacity = 0
        armed = true
        disarm.restart()
    }

    // Takes engine's frame, then runs apply (the change), if any. Frame already held or
    // taken: change runs at once.
    function take(apply) {
        if (grabbing) {
            if (apply) {
                pendingApply = apply
            }
            return
        }
        if (holding || armed || !usable()) {
            if (apply) {
                apply()
            }
            return
        }
        var source = view
        var size = Qt.size(view.width, view.height)
        var asked = ++ticket
        pendingApply = apply ? apply : null
        grabbing = true
        if (!view.grabToImage(function (result) { hold.grabbed(asked, source, size, result) },
                              size)) {
            grabbing = false
            runPending()
        }
    }

    function grabbed(asked, source, size, result) {
        if (asked !== ticket) {
            return
        }
        grabbing = false
        // Last hold still fading: its frame goes now.
        if (!holding) {
            fade.stop()
            opacity = 0
        }
        // View changed or resized before frame came: it may be stretched one. Colour only.
        var current = view === source && view.width === size.width && view.height === size.height
        shot = current ? result : null
        frameSize = size
        if (current && !holding) {
            armed = true
            disarm.restart()
        }
        runPending()
    }

    function runPending() {
        var apply = pendingApply
        pendingApply = null
        if (apply) {
            apply()
            // Change left view's size: nothing to hold.
            if (!holding) {
                armed = false
                disarm.stop()
            }
        }
    }

    function viewResized() {
        if (holding) {
            staleAspect = aspect(area.width, area.height)
            settle.stop()
        } else if (armed || grabbing) {
            if (grabbing) {
                shot = null
            }
            armed = false
            disarm.stop()
            staleAspect = aspect(area.width, area.height)
            holding = true
            fade.stop()
            opacity = 1
            patienceTimer.restart()
        }
    }

    function checkArea() {
        var shape = aspect(area.width, area.height)
        if (holding && shape > 0 && Math.abs(shape - staleAspect) > shape / 500) {
            settle.restart()
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

    // Other tab: its own frame, nothing of this one's. Change waiting on frame runs now.
    function drop() {
        ++ticket
        holding = false
        armed = false
        grabbing = false
        settle.stop()
        patienceTimer.stop()
        disarm.stop()
        fade.stop()
        opacity = 0
        shot = null
        runPending()
    }

    objectName: "frameHold"
    opacity: 0
    visible: opacity > 0 && !!view && view.visible
    clip: true

    onViewChanged: drop()
    onAreaChanged: checkArea()
    onInterruptedChanged: {
        if (interrupted) {
            release()
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

        interval: 100
        onTriggered: hold.release()
    }

    Timer {
        id: patienceTimer

        interval: hold.patience
        onTriggered: hold.release()
    }

    // Turn or change that never came.
    Timer {
        id: disarm

        interval: 1000
        onTriggered: {
            hold.armed = false
            if (!hold.holding) {
                hold.shot = null
            }
        }
    }

    // Heavy page goes on redrawing a second or two after (device): slow fade lets it settle.
    NumberAnimation {
        id: fade

        objectName: "frameHoldFade"
        target: hold
        property: "opacity"
        to: 0
        duration: 600
        onRunningChanged: {
            if (!running && !hold.holding) {
                hold.shot = null
            }
        }
    }

    Rectangle {
        objectName: "frameHoldFill"
        anchors.fill: parent
        color: hold.view && hold.view.backgroundColor ? hold.view.backgroundColor
                                                      : Theme.highlightDimmerColor
    }

    Image {
        objectName: "frameHoldImage"
        width: hold.frameSize.width
        height: hold.frameSize.height
        source: hold.shot ? hold.shot.url : ""
    }
}
