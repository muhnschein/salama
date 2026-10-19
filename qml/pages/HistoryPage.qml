// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

Page {
    id: historyPage

    objectName: "historyPage"
    allowedOrientations: Orientation.Portrait

    function open(url, inNewTab) {
        var browser = pageStack.find(function (page) {
            return page.objectName === "browserPage"
        })
        if (inNewTab || !browser) {
            TabModel.newTab(url)
        } else {
            browser.openUrl(url)
        }
        pageStack.pop(browser)
    }

    // One flickable owns pulley, so it opens above heading; on list it opened below field.
    // Doesn't scroll itself: only list moves.
    SilicaFlickable {
        objectName: "historyPulleyHost"
        anchors.fill: parent
        // Content 0x0 until told; children anchor to content.
        contentWidth: width
        contentHeight: height

        PullDownMenu {
            objectName: "historyPulley"

            MenuItem {
                objectName: "clearHistoryMenu"
                text: qsTr("Clear history")
                enabled: HistoryModel.count > 0
                onClicked: Remorse.popupAction(historyPage, qsTr("Clearing history"), function () {
                    HistoryModel.clear()
                })
            }
        }

        // Field above list, not its header: each keystroke resets model, list then focuses its
        // new current item (field loses keyboard) and scrolls content under field.
        Column {
            id: heading

            objectName: "historyHeading"
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            PageHeader {
                title: qsTr("History")
            }

            SearchField {
                objectName: "historySearch"
                width: parent.width
                placeholderText: qsTr("Search history")
                onTextChanged: HistoryModel.searchTerm = text
            }
        }

        SilicaListView {
            id: historyList

            objectName: "historyList"
            anchors {
                top: heading.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            // Rows otherwise draw over field when scrolled.
            clip: true
            model: HistoryModel

            delegate: HistoryDelegate {
                onClicked: historyPage.open(model.url, false)
                onOpenInNewTabRequested: historyPage.open(model.url, true)
                onRemoveRequested: HistoryModel.remove(index)
            }

            ViewPlaceholder {
                enabled: HistoryModel.count === 0
                text: HistoryModel.searchTerm.length > 0 ? qsTr("No matches")
                                                         : qsTr("No history yet")
            }

            VerticalScrollDecorator {}
        }
    }

    // Term is this page's: next visit starts unfiltered, as its field starts empty.
    Component.onDestruction: HistoryModel.searchTerm = ""
}
