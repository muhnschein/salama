// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Bindings, not Column: Column lays out next frame; page sizes from this height now.
import QtQuick 2.6

Item {
    id: banners

    // False while address edited, find open, or grid out.
    property bool allowed: true

    function tabOpened(tabId, title) {
        tabBanner.opened(tabId, title)
    }

    objectName: "barBanners"
    height: (tabBanner.visible ? tabBanner.height : 0)
            + (downloadBanner.visible ? downloadBanner.height : 0)

    TabBanner {
        id: tabBanner

        width: parent.width
        allowed: banners.allowed
    }

    DownloadBanner {
        id: downloadBanner

        y: tabBanner.visible ? tabBanner.height : 0
        width: parent.width
        allowed: banners.allowed
    }
}
