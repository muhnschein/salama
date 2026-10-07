// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Silica has none: PullDownMenu.menuIndicator is compat-only and logs unsupported.
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: handle

    // Finger on handle's gesture.
    property bool active: false

    objectName: "dragHandle"
    width: Theme.itemSizeMedium
    height: Theme.paddingSmall
    radius: height / 2
    color: handle.active ? Theme.highlightColor : Theme.primaryColor
    opacity: handle.active ? 1.0 : Theme.opacityHigh
}
