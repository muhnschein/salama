// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Fixed-size bottom sheet's content. Pull down only: overscroll drives sheet, y cancels it so
// content stays under finger. No drag up, fling or quick scroll; Qt 5.6 has no one-sided
// bounds, so clamp. Not AutoFlick: content fits, and auto with nothing to scroll won't drag.
import QtQuick 2.6
import Sailfish.Silica 1.0

SilicaFlickable {
    readonly property real overscroll: Math.max(0, originY - contentY)

    y: -overscroll
    flickableDirection: Flickable.VerticalFlick
    boundsBehavior: Flickable.DragOverBounds
    maximumFlickVelocity: 0
    quickScroll: false
    onContentYChanged: {
        if (contentY > originY) {
            contentY = originY
        }
    }
}
