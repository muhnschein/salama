// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Omnibar pane: ranked hits list bottom-up above go/search rows, near thumb. Glass ground
// takes every press; tap on bare glass dismisses.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: pane

    // Inactive -> model queried for nothing.
    property bool active: false
    property string text
    property bool forNewTab: false
    readonly property string typed: text.trim()
    readonly property string address: SearchSettings.isAddress(typed) ? SearchSettings.urlForInput(typed) : ""
    readonly property string engineName: SearchEngines.engineNames[SearchSettings.engineIndex]

    signal goRequested(string url)
    signal searchRequested(string url)
    signal tabChosen(int tabId)
    signal urlChosen(string url)
    signal downloadChosen(int downloadId, bool done)
    signal dismissed()

    objectName: "omnibarView"
    visible: active

    function sync() {
        if (active) {
            Omnibar.bookmarksWhenEmpty = forNewTab
            debounce.restart()
        } else {
            debounce.stop()
            Omnibar.query = ""
            Omnibar.bookmarksWhenEmpty = false
        }
    }

    function choose(kind, tabId, url, downloadId, downloadStatus) {
        if (kind === "download") {
            downloadChosen(downloadId, downloadStatus === DownloadModel.Done)
            return
        }
        Omnibar.learn(typed, url)
        if (kind === "tab") {
            tabChosen(tabId)
        } else {
            urlChosen(url)
        }
    }

    // Addresses learnt, searches not.
    function enter(text) {
        var url = SearchSettings.urlForInput(text)
        if (SearchSettings.isAddress(text)) {
            Omnibar.learn(text, url)
        }
        return url
    }

    function go() {
        goRequested(enter(typed))
    }

    onActiveChanged: sync()
    onForNewTabChanged: sync()
    onTextChanged: {
        if (active) {
            debounce.restart()
        }
    }

    // Shorter debounce than grid search: first hit is next tap.
    Timer {
        id: debounce

        objectName: "omnibarDebounce"
        interval: 150
        onTriggered: {
            if (pane.active) {
                Omnibar.query = pane.text
            }
        }
    }

    MouseArea {
        objectName: "omnibarGround"
        anchors.fill: parent
        onClicked: pane.dismissed()

        Rectangle {
            objectName: "omnibarTint"
            anchors.fill: parent
            color: Theme.highlightDimmerColor
        }

        GlassTexture {
            anchors.fill: parent
        }
    }

    SilicaListView {
        id: results

        objectName: "omnibarResults"
        anchors {
            left: parent.left
            right: parent.right
            bottom: actions.top
        }
        height: Math.max(0, Math.min(contentHeight, actions.y))
        visible: Omnibar.count > 0
        clip: true
        // Current item would steal field focus.
        currentIndex: -1
        verticalLayoutDirection: ListView.BottomToTop
        model: Omnibar

        delegate: OmnibarResultRow {
            onClicked: pane.choose(model.kind, model.tabId, model.url, model.downloadId,
                                   model.downloadStatus)
        }

        onDragStarted: Qt.inputMethod.hide()

        VerticalScrollDecorator {}
    }

    Column {
        id: actions

        objectName: "omnibarActions"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        OmnibarAction {
            objectName: "omnibarGoAction"
            width: parent.width
            visible: pane.address.length > 0
            iconSource: "image://theme/icon-m-right"
            //: The row above the address bar that opens what was typed as an address
            title: qsTr("Go to %1").arg(pane.typed)
            subtitle: pane.address
            onClicked: pane.go()
        }

        OmnibarAction {
            objectName: "omnibarSearchAction"
            width: parent.width
            visible: pane.typed.length > 0
            iconSource: "image://theme/icon-m-search"
            //: The row above the address bar that searches the web: %1 is the search
            //: engine's name, %2 what was typed
            title: qsTr("Search %1 for “%2”").arg(pane.engineName).arg(pane.typed)
            onClicked: pane.searchRequested(SearchSettings.searchUrl(pane.typed))
        }
    }
}
