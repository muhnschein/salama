// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The grab handle on an edge that can be dragged: the navigation bar, and the top of
// the tab grid. Silica draws nothing of its own here -- PullDownMenu keeps a
// menuIndicator property only for compatibility and logs that it is no longer
// supported -- and the movement that stood in for it said too little on device, so
// this is drawn and this is where the finger goes
// (docs/DECISIONS/0010-tab-grid-deck.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    id: handle

    // True while the gesture this handle belongs to has a finger on it.
    property bool active: false

    objectName: "dragHandle"
    width: Theme.itemSizeMedium
    height: Theme.paddingSmall
    radius: height / 2
    color: handle.active ? Theme.highlightColor : Theme.primaryColor
    opacity: handle.active ? 1.0 : Theme.opacityHigh
}
