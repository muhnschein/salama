// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bottom sheet's band in landscape, shared by menu and link sheet so their actions line up:
// head at left, a separator, then nine equal cells in one row with one more separator among
// them. Portrait: no band, the sheets stack head over rows.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    // Page's size.
    property real width: 0
    property real height: 0

    readonly property bool wide: width > height
    // Room a vertical separator takes.
    readonly property real gap: Theme.paddingLarge
    // Whole pixels, so head, cells and separators add up to the width exactly.
    readonly property real cell: Math.floor((width * 0.72 - 2 * gap) / 9)
    readonly property real headWidth: width - 9 * cell - 2 * gap
}
