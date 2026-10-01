// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the browsing page sets on each engine view beyond its page: the chrome gesture
// the bar reads, and the safe area for the cutout.
//
// The engine's chrome gesture is what tells the bar which way a page is being scrolled
// (docs/DECISIONS/0009-navigation-bar-gesture.md). The threshold is how far it must be
// scrolled before the engine decides; its default is zero, which flips on the first
// pixel of every drag. It is a constant rather than the bar's own height, which changes
// when the bar answers it.
//
// Through Binding rather than as properties of their own, because they belong to the
// engine's view -- a build without them should cost a warning in the log, not a page
// that fails to load.
import QtQuick 2.6
import Sailfish.Silica 1.0

QtObject {
    id: chrome

    property Item view: null
    // How far the view is kept below the cutout.
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

    // The platform's WebView hands the engine a safe area for the cutout, so that a page
    // written for one can lay itself out around it. With the view already below the
    // cutout there is nothing left for a page to avoid, and a page that did would be
    // avoiding it twice (docs/DECISIONS/0013-screen-cutout.md).
    property Binding safeArea: Binding {
        target: chrome.view
        property: "safeAreaTop"
        value: 0
        when: chrome.cutoutInset > 0
    }
}
