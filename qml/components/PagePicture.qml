// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Page's picture for tab grid, taken as load ends and as page is left. Load can end before
// engine's first paint of new document: picture then waits for it, else it showed old page or
// none. Held frame is engine's stretched one: none taken from it. Leaving, grid drag or
// background: only if page changed since its picture; each switch took one, and readback,
// encode and the model's refresh after made a switch four times the work (measured).
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: picture

    property Item view: null
    property int pageTabId: 0
    property bool current: false
    property bool held: false
    property bool waiting: false
    // Picture shows page as it is: none since load start, address change in page, scroll, zoom
    // or resize -> false.
    property bool fresh: false

    readonly property bool loading: !!view && view.loading === true
    readonly property bool painted: !!view && view.painted === true
    readonly property bool moved: !!view && (view.moving === true || view.pinching === true)
    readonly property size shape: view ? Qt.size(view.width, view.height) : Qt.size(0, 0)
    readonly property string address: view ? view.url.toString() : ""

    function capture() {
        if (!view || !current || held) {
            return
        }
        var tabId = pageTabId
        // Before grab: its result may come back inside call. Loading page still changes.
        fresh = !loading
        // Half size: GPU readback at gesture start. Encode/write off GUI thread in model.
        if (view.grabToImage(function (result) {
            if (!TabModel.storeThumbnail(tabId, result.image)) {
                picture.fresh = false
            }
        }, Qt.size(view.width / 2, view.height / 2)) !== true) {
            fresh = false
        }
    }

    function refresh() {
        if (!fresh) {
            capture()
        }
    }

    onMovedChanged: {
        if (moved) {
            fresh = false
        }
    }
    onShapeChanged: fresh = false
    onAddressChanged: fresh = false

    onLoadingChanged: {
        waiting = false
        if (loading) {
            fresh = false
        } else if (view.painted === false) {
            waiting = true
        } else {
            capture()
        }
    }

    onPaintedChanged: {
        if (painted && waiting) {
            waiting = false
            capture()
        }
    }
}
