// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The tab grid's head row under a finger. A drag down it brings the page back however
// far the grid is scrolled: from anywhere else on a grid longer than the screen, a drag
// down scrolls the grid back to its top before it can overscroll, and in a long group
// that was the whole group to scroll through to get back to the page.
//
// The drag is claimed short of the grid's own drag distance, as a cell claims a slide:
// past it the grid has taken the drag, and a flickable cannot be handed one back
// (docs/DECISIONS/0010-tab-grid-deck.md). A drag up is left to the grid, to scroll.
//
// It lies over the search field, so a tap it takes is handed on to the field. Not
// while the field has the keyboard, when a press on it places the cursor, and not a
// press on the field's clear button, which is left to the button.
import QtQuick 2.6

MouseArea {
    id: gesture

    // The field the row carries.
    property Item field

    // Where the press went down, in the window's own coordinates: the row rides on the
    // deck, which follows the finger down.
    property point pressedAt
    property bool pulling: false
    // Gone further than a tap since the press, so the click that may follow the
    // release is not one.
    property bool moved: false

    // The same signals as the grid's own pull, with the distance the finger has come
    // down since the press.
    signal pullStarted()
    signal pulled(real distance)
    signal pullFinished(real distance)

    enabled: !field || !field.activeFocus
    preventStealing: pulling

    function scenePoint(mouse) {
        return gesture.mapToItem(null, mouse.x, mouse.y)
    }

    // Silica's field carries its clear button as its right item, shown while there is
    // text to clear.
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
        // Qt's drag distance rather than Silica's, since that is the one the grid
        // measures by.
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
    // The grab can be taken away mid-pull -- by the system's edge gesture, most of all.
    // Finish at zero, so the grid springs back rather than hanging.
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
