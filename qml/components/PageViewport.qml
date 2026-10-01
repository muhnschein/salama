// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Whether a page asked for the whole screen, cutout and all, with viewport-fit=cover in
// its viewport meta tag: asked of the page as each load ends, for the reason its theme
// colour is -- sailfish-browser reads it from its own web page item, which the WebView
// Harbour allows does not have -- and forgotten as the next load starts
// (docs/DECISIONS/0043-notch-guard-modes.md).
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: viewport

    property Item view: null
    property bool coversCutout: false

    function fetch() {
        view.runJavaScript(EngineMessages.viewportScript, function (value) {
            viewport.coversCutout = EngineMessages.coversCutout(value)
        }, function () {
            viewport.coversCutout = false
        })
    }

    property Connections loads: Connections {
        target: viewport.view
        onLoadingChanged: {
            if (viewport.view.loading) {
                viewport.coversCutout = false
            } else {
                viewport.fetch()
            }
        }
    }
}
