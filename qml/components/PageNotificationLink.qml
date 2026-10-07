// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Page <-> WebNotifications: frame script relay, Notification shim, replies run in page.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    property Item view
    property int pageTabId: 0
    // qtmozembed refuses scripts before view initialized (QMozViewPrivate::runJavaScript).
    property bool ready: false

    // Again after load in case url-change try was too early. Idempotent.
    function install() {
        if (ready) {
            view.runJavaScript(WebNotifications.pageScript)
        }
    }

    // Connections claims all handlers for its target, so held, not this object.
    property Connections page: Connections {
        target: link.view
        onViewInitialized: link.ready = true
        onUrlChanged: link.install()
        onLoadingChanged: {
            if (!link.view.loading) {
                link.install()
            }
        }
        onRecvAsyncMessage: {
            if (message === WebNotifications.messageName) {
                WebNotifications.receive(link.pageTabId, data)
            }
        }
        onAboutToOpenPopup: WebNotifications.popupOpening(String(link.view.url), topic, data)
    }

    property Connections replies: Connections {
        target: WebNotifications
        onPageRequested: {
            if (tabId === link.pageTabId && link.ready) {
                link.view.runJavaScript(script)
            }
        }
    }

    // Messages heard only once listened for; qtmozembed holds frame script until view made.
    Component.onCompleted: {
        view.addMessageListener(WebNotifications.messageName)
        view.loadFrameScript(WebNotifications.relayScriptUrl)
    }
    Component.onDestruction: WebNotifications.forgetTab(pageTabId)
}
