// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The start page in the browsing page, where a view would be: made while the tab in
// front has no address, and only then -- it is read again whenever the history
// changes, which is on every page (docs/DECISIONS/0032-start-page.md). What is opened
// from it goes to the browsing page to open in the tab.
import QtQuick 2.6
import harbour.salama 1.0

Loader {
    id: startLayer

    signal openRequested(string url)

    objectName: "startPageLayer"
    active: TabModel.activeTabId > 0 && TabModel.activeUrl.length === 0
    sourceComponent: Component {
        StartPageView {
            onOpenRequested: startLayer.openRequested(url)
            onNewTabRequested: TabModel.newTab(url)
        }
    }

    // The start page's picture for the grid and the cover, taken as a page's is: at half
    // size, into the path the model hands out.
    function capture() {
        if (!item) {
            return
        }
        var tabId = TabModel.activeTabId
        var path = TabModel.thumbnailPath(tabId)
        if (path.length === 0) {
            return
        }
        item.grabToImage(function (result) {
            if (result.saveToFile(path)) {
                TabModel.updateThumbnail(tabId, path)
            }
        }, Qt.size(width / 2, height / 2))
    }
}
