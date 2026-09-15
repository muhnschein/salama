// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// The hint that an edge can be dragged: the short, centred bar Silica puts at the edge
// of a page that has a pulley menu.
//
// Silica draws its own with MenuIndicator from Sailfish.Silica.private, which Harbour
// does not allow (ci/harbour/allowed_qmlimports.conf lists Sailfish.Silica and no
// submodule of it), so this stands in for it. Neither edge here is a real PullDownMenu
// in any case: the navigation bar drags the tab grid up from under the page, and the
// grid drags the page back down over itself (docs/DECISIONS/0010-tab-grid-deck.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

Rectangle {
    width: Theme.itemSizeMedium
    height: Theme.paddingSmall / 2
    radius: height / 2
    color: Theme.highlightColor
    opacity: Theme.opacityHigh
}
