// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    id: window

    objectName: "applicationWindow"
    allowedOrientations: Orientation.Portrait
    initialPage: Component {
        BrowserPage {}
    }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    // What the cover's action asks for: a new tab with the address field open.
    //
    // Here rather than on the cover because the field belongs to the browsing page,
    // and a cover has neither the page nor the page stack in its scope. Whatever is on
    // top -- the menu, settings, a dialog -- is popped first: the action asks to type
    // an address, and it would arrive under a page that cannot.
    function requestNewTab() {
        var page = pageStack.find(function (candidate) {
            return candidate.objectName === "browserPage"
        })
        if (page) {
            pageStack.pop(page)
            page.newTabForAddress()
        }
        activate()
    }
}
