// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Between one page and PageMedia: what PageMedia asks of this page, or of every loaded
// page, is run in it as a script, and the answer is handed back. The script also carries
// out a command -- pause, play -- and mutes the page's media while its tab is muted. A
// page that starts loading takes what it played with it
// (docs/DECISIONS/0026-media-controls.md).
//
// Not an Item: it lives inside the engine's view, which draws what it has itself.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    // The engine's view of the page, and the tab it shows.
    property Item view
    property int pageTabId: 0
    readonly property bool loading: view !== null && view.loading === true

    // What PageMedia asks. A Connections takes every handler in it for its target's,
    // so it is held here rather than being this object.
    property Connections requests: Connections {
        target: PageMedia
        onRequested: {
            if (tab !== 0 && tab !== link.pageTabId) {
                return
            }
            var id = link.pageTabId
            link.view.runJavaScript(PageMedia.script(id, command), function (answer) {
                PageMedia.answer(id, command, answer)
            }, function () {
                PageMedia.answer(id, command, "")
            })
        }
    }

    onLoadingChanged: {
        if (loading) {
            PageMedia.forget(pageTabId)
        }
    }
}
