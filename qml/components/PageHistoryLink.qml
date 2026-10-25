// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Tab's back entries for back hold, read by frame script (TabHistory).
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view
    // Answer to last ask(). Stale once page moves: asked again on each back press.
    property var entries: []

    function ask() {
        entries = []
        view.sendAsyncMessage(TabHistory.askMessage, {})
    }

    function go(entry) {
        view.sendAsyncMessage(TabHistory.goMessage,
                              { "index": entry.index, "address": entry.address })
    }

    // Connections claims all handlers for its target, so held, not this object.
    property Connections page: Connections {
        target: link.view
        onRecvAsyncMessage: {
            if (message === TabHistory.resultMessage) {
                link.entries = TabHistory.entries(data)
            }
        }
    }

    // Messages heard only once listened for; qtmozembed holds frame script until view made.
    Component.onCompleted: {
        view.addMessageListener(TabHistory.resultMessage)
        view.loadFrameScript(TabHistory.scriptUrl)
    }
}
