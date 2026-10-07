// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Loaded only while front tab has no url: start page re-reads history on every page change.
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

    function capture() {
        if (!item) {
            return
        }
        var tabId = TabModel.activeTabId
        item.grabToImage(function (result) {
            TabModel.storeThumbnail(tabId, result.image)
        }, Qt.size(width / 2, height / 2))
    }
}
