// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The banners on the navigation bar, one above the other: what the downloads are doing
// (docs/DECISIONS/0038-download-status.md) and where a link opened behind the page went
// (0046-link-menu.md). As tall as the ones showing are, which is what the browsing page
// takes off the page's height, so that it ends where they begin. Laid out by bindings
// rather than a Column, whose layout waits for the next frame: the page is sized from
// this height, and has to be sized from what is showing now.
import QtQuick 2.6

Item {
    id: banners

    // Whether the page leaves room for them: not while the address is edited, a word is
    // looked for, or the grid is out.
    property bool allowed: true

    // A link was opened in a tab behind the page in front, under this name.
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
