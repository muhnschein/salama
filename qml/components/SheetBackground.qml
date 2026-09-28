// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The ground of the two sheets that come up from the foot of the screen, the browser's
// menu and the grid's list of closed tabs, so that the two read as one kind of thing
// (docs/DECISIONS/0021-menu-sheet.md, 0018-recently-closed.md). Opaque, in the tint and
// the glass the grid's rows and the navigation bar are drawn in: the sheets were see-
// through, as Silica's own PanelBackground is, and the page or the cells showing through
// were one more thing to read past what the sheet offers. Along its top edge a hairline
// and a faint glow of the highlight colour fading down from it, so that the sheet's edge
// is seen against a page that is as dark as it is.
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
