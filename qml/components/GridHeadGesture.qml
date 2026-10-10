// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Drag down on head returns to page at any scroll. Claimed below grid's drag distance:
// flickable can't hand drag back. Taps forwarded to search field, except while it has
// keyboard or on clear button.
import QtQuick 2.6

MouseArea {
    id: gesture

    property Item field

    // Page coords: row rides deck, which follows finger; window axes turn with landscape page.
    property point pressedAt
    property bool pulling: false
    property bool moved: false

    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)

    enabled: !field || !field.activeFocus
    preventStealing: pulling

    // Nearest page: still while deck moves, upright in either orientation.
    function frame() {
        var item = gesture.parent
        while (item && item.allowedOrientations === undefined) {
            item = item.parent
        }
        return item
    }

    function scenePoint(mouse) {
        return gesture.mapToItem(frame(), mouse.x, mouse.y)
    }

    // Silica field's clear button = rightItem, shown when text present.
    function overClearButton(mouse) {
        var clear = field ? field.rightItem : null
        if (!clear || !clear.visible || !clear.enabled || field.text.length === 0) {
            return false
        }
        var at = gesture.mapToItem(clear, mouse.x, mouse.y)
        return clear.contains(Qt.point(at.x, at.y))
    }

    onPressed: {
        if (overClearButton(mouse)) {
            mouse.accepted = false
            return
        }
        pressedAt = scenePoint(mouse)
        pulling = false
        moved = false
    }
    onPositionChanged: {
        var at = scenePoint(mouse)
        var across = Math.abs(at.x - pressedAt.x)
        var down = at.y - pressedAt.y
        // Qt's drag distance, not Silica's: grid measures by Qt's.
        var claim = Qt.styleHints.startDragDistance * 0.75
        if (Math.max(across, Math.abs(down)) > claim) {
            moved = true
        }
        if (!pulling && down > claim && down >= across) {
            pulling = true
            gesture.pullStarted()
        }
        if (pulling) {
            gesture.pulled(Math.max(0, down))
        }
    }
    onReleased: {
        if (pulling) {
            pulling = false
            gesture.pullFinished(Math.max(0, scenePoint(mouse).y - pressedAt.y))
        }
    }
    // Grab stolen mid-pull (system edge gesture). Finish at 0 so grid springs back.
    onCanceled: {
        if (pulling) {
            pulling = false
            gesture.pullFinished(0)
        }
    }
    onClicked: {
        if (!moved && field) {
            field.forceActiveFocus()
        }
    }
}
