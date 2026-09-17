// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 tuuli contributors
//
// Every open tab whose title or address contains what is typed, group by group. A tap
// brings that tab to the front and puts the grid away (docs/DECISIONS/0015-tab-groups.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.tuuli 1.0
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

    SilicaListView {
        id: resultList

        objectName: "tabSearchList"
        anchors.fill: parent
        model: TabSearch
        header: Column {
            width: parent.width

            PageHeader {
                title: qsTr("Search tabs")
            }

            SearchField {
                objectName: "tabSearchField"
                width: parent.width
                placeholderText: qsTr("Search tabs")
                onTextChanged: TabSearch.searchTerm = text
            }
        }

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
