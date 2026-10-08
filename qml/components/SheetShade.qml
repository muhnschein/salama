// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Dim behind open bottom sheet; tap above sheet closes it. Not DockedPanel's modal: that
// shades window-wide outside panel, so handle and lifted picture over it came out see-through.
// Dim sits under sheet (sheet needs z over page's other items); tap catcher over it, since
// without modal the panel still takes taps outside itself and drops them.
import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: shade

    property Item edgeOf
    property real strength: Theme.opacityHigh

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

    MouseArea {
        objectName: "sheetShadeTap"
        parent: shade.parent
        z: shade.edgeOf.z + 1
        width: shade.width
        height: Math.max(0, shade.edgeOf.y)
        enabled: shade.edgeOf.open
        onClicked: shade.edgeOf.hide()
    }
}
