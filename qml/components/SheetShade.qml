// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Dim behind open bottom sheet; press beside sheet closes it. DockedPanel's modal does both,
// but dims window-wide outside panel, so handle and lifted picture over it came out
// see-through. Same press catcher as modal (InverseMouseArea, window level; plain MouseArea
// over page never got presses), own dim under sheet (sheet needs z over page's other items).
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
