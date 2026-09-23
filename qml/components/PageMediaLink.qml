// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Between one page and PageMedia: what PageMedia asks of this page, or of every loaded
// page, is run in it as a script, and the answer is handed back. The script also carries
// out a command -- pause, play -- and mutes the page's media while its tab is muted
// (docs/DECISIONS/0023-media-controls.md).
import QtQuick 2.6
import harbour.salama 1.0

Connections {
    // The engine's view of the page, and the tab it shows.
    property Item view
    property int pageTabId: 0

    target: PageMedia
    onRequested: {
        if (tab !== 0 && tab !== pageTabId) {
            return
        }
        var id = pageTabId
        view.runJavaScript(PageMedia.script(id, command), function (answer) {
            PageMedia.answer(id, command, answer)
        }, function () {
            PageMedia.answer(id, command, "")
        })
    }
}
