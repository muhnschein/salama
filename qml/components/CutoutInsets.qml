// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Cutout = Silica y + height: cutout need not start at top.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    property Item view: null

    readonly property real height: Screen.topCutout
                                   ? Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
                                   : 0
    // Non-page UI (grid head row, address pane): clear of cutout unless guard disabled.
    readonly property real inset: Settings.cutoutGuard ? height : 0
    // Forced: always below cutout. Disabled: never. Default: below unless page sets
    // viewport-fit=cover; engine then reports cutout via safe area.
    readonly property real pageInset: {
        if (Settings.notchGuard === Settings.NotchGuardAutomatic) {
            return view && view.viewport && view.viewport.coversCutout ? 0 : height
        }
        return Settings.notchGuard === Settings.NotchGuardForced ? height : 0
    }
}
