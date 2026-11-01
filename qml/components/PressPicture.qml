// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bar press grabs page picture while finger still (grab in drag's first frame stuttered), but
// stores it only once drag starts. Tap drops it: storing wrote JPEG, saved tab, reloaded grid
// tile on every bar tap (menu, back, address), ~60 ms stall in tap's animation.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    // "idle": late picture dropped; "held": kept for drag; "kept": stored on arrival.
    property string phase: "idle"
    property int tabId: -1
    property var image: null

    function arm() {
        phase = "held"
        image = null
    }

    function take(id, picture) {
        if (phase === "kept") {
            TabModel.storeThumbnail(id, picture)
        } else if (phase === "held") {
            tabId = id
            image = picture
        }
    }

    function keep() {
        if (phase === "held" && image !== null) {
            TabModel.storeThumbnail(tabId, image)
        }
        phase = "kept"
        image = null
    }

    function drop() {
        phase = "idle"
        image = null
    }
}
