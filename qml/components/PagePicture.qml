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
    // Load-end picture held off while page moves: readback mid-fling drops frames and pictures
    // scroll page leaves behind.
    property bool deferred: false
    // Picture shows page as it is: none since load start, address change in page, scroll, zoom
    // or resize -> false.
    property bool fresh: false

    readonly property bool loading: !!view && view.loading === true
    readonly property bool painted: !!view && view.painted === true
    readonly property bool moved: !!view && (view.moving === true || view.pinching === true)
    readonly property size shape: view ? Qt.size(view.width, view.height) : Qt.size(0, 0)
    readonly property string address: view ? view.url.toString() : ""

    function capture() {
        deferred = false
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

    function take() {
        if (moved) {
            deferred = true
        } else {
            capture()
        }
    }

    // Either way: picture taken mid-move shows page short of where it rests.
    onMovedChanged: {
        fresh = false
        if (!moved && deferred) {
            settle.restart()
        }
    }
    onShapeChanged: fresh = false
    onAddressChanged: fresh = false

    onLoadingChanged: {
        waiting = false
        if (loading) {
            fresh = false
            deferred = false
            settle.stop()
        } else if (view.painted === false) {
            waiting = true
        } else {
            take()
        }
    }

    onPaintedChanged: {
        if (painted && waiting) {
            waiting = false
            take()
        }
    }

    // Engine's last frames of fling land after moving ends. Moving again: waits for next rest.
    property Timer settle: Timer {
        interval: 200
        onTriggered: {
            if (picture.deferred && !picture.moved) {
                picture.capture()
            }
        }
    }
}
