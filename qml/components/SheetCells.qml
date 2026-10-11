// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Link sheet in landscape: head at left, a separator, then portrait's rows stacked in the block
// beside it. Portrait: head over rows.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    // Page's size.
    property real width: 0
    property real height: 0

    readonly property bool wide: width > height
    // Room a vertical separator takes.
    readonly property real gap: Theme.paddingLarge
    // Rows' width: a multiple of 4 px, so quarters are whole pixels and head, separator and rows
    // add up to the width exactly.
    readonly property real block: Math.floor((width * 0.72 - gap) / 4) * 4
    readonly property real headWidth: width - gap - block
}
