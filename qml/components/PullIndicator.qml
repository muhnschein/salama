// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The hint that an edge can be dragged: three dots along it, the way a Silica pulley
// sits at rest. Drawn here rather than taken from Silica because neither edge is a
// real PullDownMenu -- the navigation bar drags the tab grid up from under the page,
// and the grid drags the page back down over itself
// (docs/DECISIONS/0010-tab-grid-deck.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Row {
    id: indicator

    property color dotColor: Theme.highlightColor

    spacing: Theme.paddingSmall

    Repeater {
        model: 3

        Rectangle {
            width: Theme.paddingSmall
            height: width
            radius: width / 2
            color: indicator.dotColor
            opacity: Theme.opacityHigh
        }
    }
}
