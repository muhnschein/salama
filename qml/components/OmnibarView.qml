// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The pane the address bar brings up above itself while it is typed into: what the
// words find among the open tabs of every group, the bookmarks, the history and the
// downloads, a section of each under a heading that says how many -- one list sectioned
// by kind, as piirit lists what its search finds -- and below them, directly above the
// bar and in reach of the thumb that typed, the rows that go to what was typed as an
// address or search the web for it (docs/DECISIONS/0027-omnibar.md). Opened for a new
// tab with nothing typed yet, it lists the bookmarks, as sailfish-browser's new-tab
// overlay lists its favourites.
//
// The list hangs from those rows and is as tall as what it holds, up to the room there
// is, so a short one sits by the bar. Under both is a pane of the grid's glass
// (docs/DECISIONS/0010-tab-grid-deck.md) that takes every press: nothing of the page it
// covers is reached through it, and a tap where it is bare puts the pane away.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: pane

    // Up above the bar. While it is not, the model is asked for nothing, and a source
    // that changes meanwhile has no list to build.
    property bool active: false
    // What is typed, and whether the bar was opened for a new tab.
    property string text
    property bool forNewTab: false
    readonly property string typed: text.trim()
    // Where Enter would go, when what is typed is an address rather than words.
    readonly property string address: Settings.isAddress(typed) ? Settings.urlForInput(typed) : ""
    readonly property string engineName: Settings.searchEngineNames[Settings.searchEngineIndex]

    signal goRequested(string url)
    signal searchRequested(string url)
    signal tabChosen(int tabId)
    signal urlChosen(string url)
    signal downloadChosen(int downloadId, bool done)
    // A tap on the bare glass.
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

    // A section's heading, with how many of how many when the section shows fewer
    // than it found.
    function heading(kind) {
        if (kind === "tab") {
            return counted(qsTr("Tabs (%1)"), qsTr("Tabs (%1 of %2)"),
                           Omnibar.tabCount, Omnibar.tabTotal)
        }
        if (kind === "bookmark") {
            return counted(qsTr("Bookmarks (%1)"), qsTr("Bookmarks (%1 of %2)"),
                           Omnibar.bookmarkCount, Omnibar.bookmarkTotal)
        }
        if (kind === "history") {
            return counted(qsTr("History (%1)"), qsTr("History (%1 of %2)"),
                           Omnibar.historyCount, Omnibar.historyTotal)
        }
        return counted(qsTr("Downloads (%1)"), qsTr("Downloads (%1 of %2)"),
                       Omnibar.downloadCount, Omnibar.downloadTotal)
    }

    function counted(all, some, shown, total) {
        return total > shown ? some.arg(shown).arg(total) : all.arg(shown)
    }

    function choose(kind, tabId, url, downloadId, downloadStatus) {
        if (kind === "tab") {
            tabChosen(tabId)
        } else if (kind === "download") {
            downloadChosen(downloadId, downloadStatus === DownloadModel.Done)
        } else {
            urlChosen(url)
        }
    }

    onActiveChanged: sync()
    onForNewTabChanged: sync()
    onTextChanged: {
        if (active) {
            debounce.restart()
        }
    }

    // Only the last of a burst of keystrokes is looked for, as in the grid's search,
    // but sooner: what is typed here is typed to go somewhere, and the first row found
    // is the next tap, where the grid's search narrows a list already on the screen.
    // The rows that go or search follow the text at once.
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

        // Opaque, as the grid's rows and the bar are: the page showing through,
        // however faintly, was one more thing to read past.
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
        // Absent while it holds nothing: the rows below say what can be done instead.
        visible: Omnibar.count > 0
        clip: true
        // As sailfish-browser's history list has it: a current item would take the
        // focus from the field (apps/browser/qml/pages/components/HistoryList.qml).
        currentIndex: -1
        model: Omnibar
        section.property: "kind"
        section.delegate: SectionHeader {
            objectName: "omnibarSection"
            text: pane.heading(section)
        }

        delegate: OmnibarResultRow {
            onClicked: pane.choose(model.kind, model.tabId, model.url, model.downloadId,
                                   model.downloadStatus)
        }

        // Scrolled, the list wants the room the keyboard takes: it is put away, as
        // Firefox's suggestions put it away, and a tap on the field brings it back.
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
            onClicked: pane.goRequested(pane.address)
        }

        // A search even when what is typed reads as an address.
        OmnibarAction {
            objectName: "omnibarSearchAction"
            width: parent.width
            visible: pane.typed.length > 0
            iconSource: "image://theme/icon-m-search"
            //: The row above the address bar that searches the web: %1 is the search
            //: engine's name, %2 what was typed
            title: qsTr("Search %1 for “%2”").arg(pane.engineName).arg(pane.typed)
            onClicked: pane.searchRequested(Settings.searchUrl(pane.typed))
        }
    }
}
