// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tab grid cells, shared by real and tutorial grid. Portrait: two square previews a row.
// Landscape: three, page shaped, so a whole row fits on the low page.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    // Grid's, page sized.
    property real width: 0
    property real height: 0
    // Page's, when grid leaves room beside it (landscape group rail).
    property real pageWidth: width

    readonly property bool wide: pageWidth > height
    readonly property int columns: wide ? 3 : 2
    readonly property real cellWidth: width / columns
    readonly property real cellHeight: (wide ? Math.round(cellWidth * height / pageWidth)
                                             : cellWidth)
                                       + Theme.itemSizeSmall
}
