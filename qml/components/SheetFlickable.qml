// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Fixed-size bottom sheet's content. Pull down only: overscroll drives sheet, y cancels it so
// content stays under finger. No drag up, fling or quick scroll; Qt 5.6 has no one-sided
// bounds, so clamp. Not AutoFlick: content fits, and auto with nothing to scroll won't drag.
// Across: side sheet's, the same pulled right, x cancelling it.
import QtQuick 2.6
import Sailfish.Silica 1.0

SilicaFlickable {
    property bool across: false
    readonly property real overscroll: across ? Math.max(0, originX - contentX)
                                              : Math.max(0, originY - contentY)

    x: across ? -overscroll : 0
    y: across ? 0 : -overscroll
    // -1: Flickable's own default.
    contentWidth: across ? width : -1
    flickableDirection: across ? Flickable.HorizontalFlick : Flickable.VerticalFlick
    boundsBehavior: Flickable.DragOverBounds
    maximumFlickVelocity: 0
    quickScroll: false
    onContentXChanged: {
        if (contentX > originX) {
            contentX = originX
        }
    }
    onContentYChanged: {
        if (contentY > originY) {
            contentY = originY
        }
    }
}
