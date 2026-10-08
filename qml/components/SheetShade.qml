// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Dim behind open bottom sheet; tap closes sheet. Not DockedPanel's modal: that shades
// window-wide outside panel, so handle and lifted picture over it came out see-through.
import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: shade

    property Item edgeOf
    property real strength: Theme.opacityHigh

    parent: edgeOf ? edgeOf.parent : null
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0
    z: edgeOf.z - 1
    enabled: edgeOf.open
    visible: opacity > 0
    opacity: edgeOf.open ? 1.0 : 0.0
    onClicked: edgeOf.hide()

    Behavior on opacity {
        FadeAnimation {}
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.highlightDimmerColor
        opacity: shade.strength
    }
}
