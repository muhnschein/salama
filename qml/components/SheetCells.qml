// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Link sheet in landscape: head at left, gap, then portrait's rows in block beside it.
// Portrait: head over rows.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    // Page's size.
    property real width: 0
    property real height: 0

    readonly property bool wide: width > height
    readonly property real gap: Theme.paddingLarge
    // Rows' width, whole pixels: head, gap and rows add up to width exactly.
    readonly property real block: Math.floor(width * 0.72 - gap)
    readonly property real headWidth: width - gap - block
}
