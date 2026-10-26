// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Landscape: Silica keeps page clear of cutout (AvoidLandscapeCutout), leaving strip beside
// it to window's wallpaper. Black there, as phones hide notch, so page meets dark edge.
// Child of page: lies outside it, on side device's top turned to.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    property Item page: parent

    visible: page.isLandscape && width > 0
    width: Screen.topCutout ? Screen.topCutout.height : 0
    height: page.height
    // Turned quarter clockwise, page's left edge is device's top; turned back, its right.
    x: page.orientation === Orientation.LandscapeInverted ? page.width : -width
    color: "black"
}
