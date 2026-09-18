// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Every open tab whose title or address contains what is typed, group by group. A tap
// brings that tab to the front and puts the grid away (docs/DECISIONS/0015-tab-groups.md).
//
// The field is anchored above the list, not in its header, and the term reaches the
// model a beat after the last keystroke, the way postivene's chat search does both.
// A header item lives inside the view's flickable, and Silica takes the keyboard
// away when a flickable's content moves under it -- which a list narrowing on every
// keystroke does; and a term written on every keystroke had the list changing under
// the finger as the first results came in.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: searchPage

    objectName: "tabSearchPage"
    allowedOrientations: Orientation.Portrait

    function open(tabId) {
        var browser = pageStack.find(function (page) {
            return page.objectName === "browserPage"
        })
        if (browser) {
            browser.showTab(tabId)
        }
        pageStack.pop(browser)
    }

    // Not bound straight to the field: only the last of a burst of keystrokes is
    // wanted.
    Timer {
        id: searchDebounce

        objectName: "searchDebounce"
        interval: 250
        onTriggered: TabSearch.searchTerm = searchField.text
    }

    Column {
        id: heading

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        PageHeader {
            title: qsTr("Search tabs")
        }

        SearchField {
            id: searchField

            objectName: "tabSearchField"
            width: parent.width
            placeholderText: qsTr("Search tabs")
            onTextChanged: searchDebounce.restart()
        }
    }

    SilicaListView {
        id: resultList

        objectName: "tabSearchList"
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        // Rows draw outside the list's own box otherwise, over the field.
        clip: true
        model: TabSearch

        delegate: TabSearchDelegate {
            onChosen: searchPage.open(model.tabId)
        }

        ViewPlaceholder {
            enabled: TabSearch.count === 0
            text: qsTr("No matching tabs")
        }

        VerticalScrollDecorator {}
    }

    // The term is this page's alone: the next search starts empty.
    Component.onDestruction: TabSearch.searchTerm = ""
}
