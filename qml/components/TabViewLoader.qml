// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Where one tab's view is made in the browsing page, which hands it the view to make:
// only once the tab has been in front this session, only while it is among the pages
// kept loaded (docs/DECISIONS/0003-one-webview-per-tab.md, 0016-five-live-pages.md),
// and only while it has a page -- a tab on the start page has none
// (docs/DECISIONS/0032-start-page.md). The view reads the tab's id, whether it is in
// front and the address to load from here.
import QtQuick 2.6
import harbour.salama 1.0

Loader {
    readonly property int tabId: model.tabId
    readonly property bool isCurrent: model.activeTab
    readonly property bool liveTab: model.liveTab
    readonly property string initialUrl: model.url
    property bool shown: false
    // The page in the view was opened from the start page, which back from its first
    // page returns to, as Firefox's does. The view knows nothing of the start page.
    property bool startPageBehind: false

    objectName: "webViewLoader"
    anchors.fill: parent
    active: shown && liveTab && initialUrl.length > 0
    visible: isCurrent
    onIsCurrentChanged: {
        if (isCurrent) {
            shown = true
        }
    }
    Component.onCompleted: {
        if (isCurrent) {
            shown = true
        }
    }

    // A page opened from the start page is opened in the tab, which a view is made for.
    function openFromStartPage(url) {
        startPageBehind = true
        TabModel.updateUrl(tabId, url)
    }

    function backToStartPage() {
        startPageBehind = false
        TabModel.showStartPage(tabId)
    }
}
