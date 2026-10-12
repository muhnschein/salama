// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Page's picture for tab grid, taken as load ends and as page is left. Load can end before
// engine's first paint of new document: picture then waits for it, else it showed old page or
// none. Held frame is engine's stretched one: none taken from it.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: picture

    property Item view: null
    property int pageTabId: 0
    property bool current: false
    property bool held: false
    property bool waiting: false

    readonly property bool loading: !!view && view.loading === true
    readonly property bool painted: !!view && view.painted === true

    function capture() {
        if (!view || !current || held) {
            return
        }
        var tabId = pageTabId
        // Half size: GPU readback at gesture start. Encode/write off GUI thread in model.
        view.grabToImage(function (result) {
            TabModel.storeThumbnail(tabId, result.image)
        }, Qt.size(view.width / 2, view.height / 2))
    }

    onLoadingChanged: {
        waiting = false
        if (!loading) {
            if (view.painted === false) {
                waiting = true
            } else {
                capture()
            }
        }
    }

    onPaintedChanged: {
        if (painted && waiting) {
            waiting = false
            capture()
        }
    }
}
