// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Opaque: see-through was one more thing to read past. Glow so edge shows over dark pages.
// Split: head (page and its actions) lighter, lists below deeper, both from ambience.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: ground

    // Top part in head shade; 0: one shade throughout.
    property real headHeight: 0
    // Head beside rows (landscape link sheet): head shade only this far from left.
    property real headWidth: width
    readonly property color headColor: Qt.tint(Theme.highlightDimmerColor,
                                               Theme.rgba(Theme.highlightBackgroundColor, 0.18))
    readonly property color bodyColor: Qt.tint(Theme.highlightDimmerColor,
                                               Theme.rgba(Theme.overlayBackgroundColor, 0.45))
    // Side sheet: edge line down its left side too.
    property bool side: false

    objectName: "sheetBackground"
    color: headHeight > 0 ? bodyColor : Theme.highlightDimmerColor

    Rectangle {
        objectName: "sheetHead"
        width: Math.min(parent.width, ground.headWidth)
        height: Math.min(parent.height, ground.headHeight)
        visible: ground.headHeight > 0
        color: ground.headColor
    }

    GlassTexture {
        anchors.fill: parent
    }

    Rectangle {
        objectName: "sheetGlow"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: Theme.itemSizeSmall
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: Theme.rgba(Theme.highlightColor, Theme.opacityFaint / 2)
            }
            GradientStop {
                position: 1.0
                color: Theme.rgba(Theme.highlightColor, 0.0)
            }
        }
    }

    Rectangle {
        objectName: "sheetEdge"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: Theme._lineWidth
        color: Theme.rgba(Theme.highlightColor, Theme.opacityFaint)
    }

    Rectangle {
        objectName: "sheetSideEdge"
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }
        width: Theme._lineWidth
        visible: ground.side
        color: Theme.rgba(Theme.highlightColor, Theme.opacityFaint)
    }
}
