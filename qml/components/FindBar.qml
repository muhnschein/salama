// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Searching the page in front, from the menu: a field laid over the navigation bar,
// with the previous and the next match and a close button beside it. What is typed
// goes to the page with Enter, and the keyboard goes with it so the match can be seen;
// the arrows then step through the matches, round the ends of the page. The engine
// marks each match and scrolls to it itself (embedlite-components
// jsscripts/embedhelper.js), and what it is told is EngineMessages'
// (docs/DECISIONS/0021-menu-sheet.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Rectangle {
    id: findBar

    // The page in front.
    property Item view: null
    // The page the bar was opened on, which is the one told when the search ends:
    // by then another page may be in front.
    property Item searched: null
    property bool active: false
    // What was last searched for, which the arrows step through.
    property string term
    // Whether the page found it; the field says so when it did not.
    property bool found: true

    objectName: "findBar"
    visible: active
    // Opaque, as the bar it lies over is.
    color: Theme.highlightDimmerColor

    function open() {
        searched = view
        term = ""
        found = true
        field.text = ""
        active = true
        field.forceActiveFocus()
    }

    // The highlight goes from the page with the search, and the next search starts
    // from its top.
    function close() {
        if (!active) {
            return
        }
        active = false
        field.focus = false
        if (searched) {
            searched.sendAsyncMessage(EngineMessages.findMessage,
                                      EngineMessages.findRequest("", false, false))
        }
        searched = null
        term = ""
    }

    function search(text) {
        field.focus = false
        if (!searched || text.length === 0) {
            return
        }
        term = text
        searched.sendAsyncMessage(EngineMessages.findMessage,
                                  EngineMessages.findRequest(text, false, false))
    }

    function step(backwards) {
        if (!searched || term.length === 0) {
            return
        }
        searched.sendAsyncMessage(EngineMessages.findMessage,
                                  EngineMessages.findRequest(term, true, backwards))
    }

    // The page's answer to each search and each step.
    function received(message, data) {
        if (message === EngineMessages.findResultMessage) {
            found = EngineMessages.findFound(data)
        }
    }

    // Another page in front -- a tab chosen, or this one closed -- and the search
    // was that one's.
    onViewChanged: close()

    Connections {
        target: findBar.searched
        onRecvAsyncMessage: findBar.received(message, data)
    }

    // The bar's presses are the bar's: the navigation bar under it keeps none of
    // them while it is covered.
    MouseArea {
        anchors.fill: parent
    }

    TextField {
        id: field

        objectName: "findField"
        anchors {
            left: parent.left
            right: previousButton.left
            leftMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        placeholderText: qsTr("Search on page")
        font.pixelSize: Theme.fontSizeMedium
        errorHighlight: !findBar.found
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        EnterKey.enabled: text.length > 0
        EnterKey.iconSource: "image://theme/icon-m-search"
        EnterKey.onClicked: findBar.search(text)
    }

    IconButton {
        id: previousButton

        objectName: "findPreviousButton"
        anchors {
            right: nextButton.left
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        enabled: findBar.term.length > 0
        icon.source: "image://theme/icon-m-left"
        onClicked: findBar.step(true)
    }

    IconButton {
        id: nextButton

        objectName: "findNextButton"
        anchors {
            right: closeButton.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        enabled: findBar.term.length > 0
        icon.source: "image://theme/icon-m-right"
        onClicked: findBar.step(false)
    }

    IconButton {
        id: closeButton

        objectName: "findCloseButton"
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: width
        icon.source: "image://theme/icon-m-close"
        onClicked: findBar.close()
    }
}
