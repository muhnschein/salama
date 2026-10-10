// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bindings, not own properties: missing in some engine build = log warning, not failed load.
// Chrome threshold default 0 flips on first pixel; constant since bar height reacts to it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

QtObject {
    id: chrome

    property Item view: null
    property real cutoutInset: 0
    // Landscape bar hides whole: tap off links and fields, and page top, bring chrome back.
    property bool revealing: false
    readonly property bool atTop: !!view && view.atYBeginning === true

    function reveal() {
        if (revealing) {
            view.chrome = true
        }
    }

    onAtTopChanged: {
        if (atTop) {
            reveal()
        }
    }

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

    // Connections claims all handlers for its target, so held, not this object.
    property Connections taps: Connections {
        target: chrome.view
        onRecvAsyncMessage: {
            if (message === EngineMessages.pageTapMessage) {
                chrome.reveal()
            }
        }
    }

    // Message heard only once listened for.
    Component.onCompleted: {
        view.addMessageListener(EngineMessages.pageTapMessage)
        view.loadFrameScript(EngineMessages.pageTapScriptUrl)
    }
}
