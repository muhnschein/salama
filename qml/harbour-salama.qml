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

    // What the cover's action asks for: the address bar opened for a new tab, empty,
    // with its pane over the page listing the bookmarks until something is typed. No
    // tab is made until something is chosen, and none if the pane is put away
    // (docs/DECISIONS/0027-omnibar.md).
    //
    // Here rather than on the cover because the bar belongs to the browsing page, and
    // a cover has neither the page nor the page stack in its scope. Whatever page is
    // on top -- settings, a dialog -- is popped first: the action asks to type an
    // address, and it would arrive under a page that cannot. The menu sheet and the
    // find bar are no pages: the browsing page puts them away as it opens the bar.
    function requestNewTab() {
        var page = pageStack.find(function (candidate) {
            return candidate.objectName === "browserPage"
        })
        if (page) {
            pageStack.pop(page)
            page.openOmnibar(true)
        }
        activate()
    }
}
