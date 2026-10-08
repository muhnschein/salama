// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Engine highlights and scrolls to matches itself (embedlite-components embedhelper.js).
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Rectangle {
    id: findBar

    property Item view: null
    // Page search started on; told on close, since front page may have changed.
    property Item searched: null
    property bool active: false
    property string term
    property bool found: true

    objectName: "findBar"
    visible: active
    color: Theme.highlightDimmerColor

    function open() {
        searched = view
        term = ""
        found = true
        field.text = ""
        active = true
        field.forceActiveFocus()
    }

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

    function received(message, data) {
        if (message === EngineMessages.findResultMessage) {
            found = EngineMessages.findFound(data)
        }
    }

    onViewChanged: close()

    Connections {
        target: findBar.searched
        onRecvAsyncMessage: findBar.received(message, data)
    }

    // Swallow presses so covered nav bar gets none.
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
            // Silica reports text offset left by label/underline room; aligns with address field.
            verticalCenterOffset: field.textVerticalCenterOffset === undefined
                                  ? 0 : field.textVerticalCenterOffset
        }
        placeholderText: qsTr("Find in page")
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
