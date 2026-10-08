// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Dim behind open bottom sheet; press beside sheet closes it. DockedPanel's modal does both,
// but dims window-wide outside panel, so handle and lifted picture over it came out
// see-through. Own dim under sheet (sheet needs z over page's other items). Two press catchers:
// modal's InverseMouseArea sees only mouse presses, and web view takes touches as touch, so dim
// also swallows presses and closes on press (not click: no click came on device).
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: shade

    property Item edgeOf
    property real strength: Theme.opacityHigh
    // Room over sheet that counts as sheet: lifted picture takes its own pinch and taps.
    property real reach: 0

    parent: edgeOf ? edgeOf.parent : null
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: edgeOf.z - 1
    visible: opacity > 0
    opacity: edgeOf.open ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {}
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
        opacity: shade.strength
    }

    // Also while fading out: press meant for sheet never lands on page.
    MouseArea {
        objectName: "sheetShadePress"
        anchors.fill: parent
        onPressed: shade.edgeOf.hide()
    }

    InverseMouseArea {
        objectName: "sheetShadeTap"
        parent: shade.edgeOf
        y: -shade.reach
        width: shade.edgeOf.width
        height: shade.edgeOf.height + shade.reach
        enabled: shade.edgeOf.open
        stealPress: true
        onPressedOutside: shade.edgeOf.hide()
    }
}
