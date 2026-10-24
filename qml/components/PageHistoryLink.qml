// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tab's back entries for back hold, read by frame script (EngineMessages.tabHistory*).
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view
    // Answer to last ask(). Stale once page moves: asked again on each back press.
    property var entries: []

    function ask() {
        entries = []
        view.sendAsyncMessage(EngineMessages.tabHistoryMessage, {})
    }

    function go(entry) {
        view.sendAsyncMessage(EngineMessages.tabHistoryGoMessage,
                              { "index": entry.index, "address": entry.address })
    }

    // Connections claims all handlers for its target, so held, not this object.
    property Connections page: Connections {
        target: link.view
        onRecvAsyncMessage: {
            if (message === EngineMessages.tabHistoryResultMessage) {
                link.entries = EngineMessages.tabHistory(data)
            }
        }
    }

    // Messages heard only once listened for; qtmozembed holds frame script until view made.
    Component.onCompleted: {
        view.addMessageListener(EngineMessages.tabHistoryResultMessage)
        view.loadFrameScript(EngineMessages.tabHistoryScriptUrl)
    }
}
