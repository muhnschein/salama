// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tab grid cells, shared by real and tutorial grid. Portrait: two square previews a row.
// Landscape: three, page shaped, so a row fits under head and above foot rows.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    // Grid's, page sized.
    property real width: 0
    property real height: 0

    readonly property bool wide: width > height
    readonly property int columns: wide ? 3 : 2
    readonly property real cellWidth: width / columns
    readonly property real cellHeight: (wide ? Math.round(cellWidth * height / width) : cellWidth)
                                       + Theme.itemSizeSmall
}
