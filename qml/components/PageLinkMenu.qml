// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Content:ContextMenu on link/image -> link sheet. Text presses left to platform selection.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view

    signal requested(var target)

    // Connections claims all handlers for its target, so held, not this object. Platform
    // WebView already listens for message (import/webview/WebView.qml).
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

    // Blank stand-in replaces platform menu.
    Component.onCompleted: {
        if (view.popupProvider) {
            view.popupProvider.contextMenu = {
                "type": "item",
                "component": String(Qt.resolvedUrl("PlatformMenuStandIn.qml"))
            }
        }
    }
}
