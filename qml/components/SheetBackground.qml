// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Opaque: see-through was one more thing to read past. Glow so edge shows over dark pages.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    objectName: "sheetBackground"
    color: Theme.highlightDimmerColor

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
}
