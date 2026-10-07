// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// viewport-fit=cover via script: Harbour-allowed WebView lacks web page item exposing it.
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
