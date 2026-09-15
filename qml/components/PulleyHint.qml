// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// What says an edge is a pulley. Silica draws no indicator for one: PullDownMenu
// keeps a menuIndicator property only for compatibility and logs that it is no longer
// supported, and the hint the platform does give is a movement -- PulleyAnimationHint
// peeks the menu open and lets it fall back. This is that movement, at its distance
// and its two durations (docs/DECISIONS/0010-tab-grid-deck.md).
import QtQuick 2.6
import Sailfish.Silica 1.0

SequentialAnimation {
    id: hint

    // The offset to borrow, which is added to wherever the view is drawn rather than
    // being the offset a spring already drives: one value, two animations, otherwise.
    property Item item
    property string offsetProperty
    property real distance: Theme.itemSizeExtraSmall

    NumberAnimation {
        target: hint.item
        property: hint.offsetProperty
        to: hint.distance
        duration: 400
        easing.type: Easing.OutCubic
    }

    NumberAnimation {
        target: hint.item
        property: hint.offsetProperty
        to: 0
        duration: 400
        easing.type: Easing.InOutCubic
    }
}
