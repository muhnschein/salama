// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the display's own cutout takes at the top of the screen, and how much of it the
// browsing page keeps out of, as the notch guard says (docs/DECISIONS/0013-screen-cutout.md,
// 0043-notch-guard-modes.md). Silica reports the cutout's whole rectangle, and it is read
// as y plus height because a cutout need not start at the very top -- sailfish-browser
// reads the same pair.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    // The page in front, or null.
    property Item view: null

    readonly property real height: Screen.topCutout
                                   ? Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
                                   : 0
    // What is not a page -- the tab grid's head row, the address bar's pane -- keeps out
    // of the cutout unless the guard is disabled.
    readonly property real inset: Settings.cutoutGuard ? height : 0
    // The page in front: always below the cutout while the guard is forced, never while
    // it is disabled, and by default unless the page asked for the whole screen with
    // viewport-fit=cover, a page written for the cutout, which the engine then tells
    // where it is through the safe area.
    readonly property real pageInset: {
        if (Settings.notchGuard === Settings.NotchGuardAutomatic) {
            return view && view.viewport && view.viewport.coversCutout ? 0 : height
        }
        return Settings.notchGuard === Settings.NotchGuardForced ? height : 0
    }
}
