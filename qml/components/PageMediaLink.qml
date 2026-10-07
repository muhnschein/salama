// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Runs PageMedia scripts in page, hands answers back. New load forgets page's media.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view
    property int pageTabId: 0
    readonly property bool loading: view !== null && view.loading === true

    // Connections claims all handlers for its target, so held, not this object.
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
