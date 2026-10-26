// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bindings, not own properties: missing in some engine build = log warning, not failed load.
// Chrome threshold default 0 flips on first pixel; constant since bar height reacts to it.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    id: chrome

    property Item view: null
    property real cutoutInset: 0

    property Binding gesture: Binding {
        target: chrome.view
        property: "chromeGestureEnabled"
        value: true
    }

    property Binding threshold: Binding {
        target: chrome.view
        property: "chromeGestureThreshold"
        value: Theme.itemSizeLarge
    }

    // View already below cutout; nonzero safe area makes page avoid it twice.
    property Binding safeArea: Binding {
        target: chrome.view
        property: "safeAreaTop"
        value: 0
        when: chrome.cutoutInset > 0
    }

    // Engine's own landscape inset: Silica already keeps landscape page clear of cutout
    // (AvoidLandscapeCutout), so page padded beside it, and each turn sent engine one more change.
    property Binding safeAreaLeft: Binding {
        target: chrome.view
        property: "safeAreaLeft"
        value: 0
    }

    property Binding safeAreaRight: Binding {
        target: chrome.view
        property: "safeAreaRight"
        value: 0
    }
}
