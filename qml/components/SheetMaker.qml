// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Sheet made after launch, slice per frame: first frame never shows it (menu, link and tab
// history sheets: about 230 items). Use before then finishes making at once.
import QtQuick 2.6

QtObject {
    id: maker

    property Component component
    property Item item: null
    property var incubator: null

    // Caller's order is stacking order once all made.
    function make(into) {
        incubator = component.incubateObject(into)
        if (incubator.status === Component.Ready) {
            item = incubator.object
        } else {
            incubator.onStatusChanged = function (status) {
                if (status === Component.Ready) {
                    maker.item = maker.incubator.object
                }
            }
        }
    }

    function get() {
        if (incubator && incubator.status === Component.Loading) {
            incubator.forceCompletion()
        }
        return item
    }
}
