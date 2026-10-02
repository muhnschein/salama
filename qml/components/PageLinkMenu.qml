// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Between one page and the link sheet: a press held on a link or a picture, which the
// engine tells of with Content:ContextMenu, is read here and handed on as what the sheet
// shows (docs/DECISIONS/0046-link-menu.md). A press on text, or on anything else, is left
// to the platform, which starts selecting text on the same message.
//
// Not an Item: it lives inside the engine's view, which draws what it has itself.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    // The engine's view of the page.
    property Item view

    // A link or a picture was pressed: what EngineMessages.linkTarget() makes of it.
    signal requested(var target)

    // A Connections takes every handler in it for its target's, so it is held here
    // rather than being this object. The platform's WebView listens for the message
    // already (import/webview/WebView.qml), so it is not asked for again.
    property Connections page: Connections {
        target: link.view
        onRecvAsyncMessage: {
            if (message === EngineMessages.contextMenuMessage) {
                var pressed = EngineMessages.linkTarget(data)
                if (pressed.link.length > 0 || pressed.image.length > 0) {
                    link.requested(pressed)
                }
            }
        }
    }

    // The platform's own menu for the message is made a stand-in that shows nothing, in
    // the shape its provider gives its own: a dictionary naming the file.
    Component.onCompleted: {
        if (view.popupProvider) {
            view.popupProvider.contextMenu = {
                "type": "item",
                "component": String(Qt.resolvedUrl("PlatformMenuStandIn.qml"))
            }
        }
    }
}
