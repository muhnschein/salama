// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Loaded once shown this session, while live (live-page limit) and while tab has url.
import QtQuick 2.6
import harbour.salama 1.0

Loader {
    readonly property int tabId: model.tabId
    readonly property bool isCurrent: model.activeTab
    readonly property bool liveTab: model.liveTab
    readonly property string initialUrl: model.url
    property bool shown: false
    // Back from first page returns to start page; view knows nothing of it.
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

    function openFromStartPage(url) {
        startPageBehind = true
        TabModel.updateUrl(tabId, url)
    }

    function backToStartPage() {
        startPageBehind = false
        TabModel.showStartPage(tabId)
    }
}
