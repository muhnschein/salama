// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Between one page and WebNotifications: the frame script that hands on what the page
// says of notifications, loaded as the view is made; the page's own
// Notification, put in place as each document arrives; and what WebNotifications answers
// the page, run in it. A page that goes takes what it showed with it
// (docs/DECISIONS/0033-web-notifications.md).
//
// Not an Item: it lives inside the engine's view, which draws what it has itself.
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    id: link

    // The engine's view of the page, and the tab it shows.
    property Item view
    property int pageTabId: 0
    // Whether the engine has made the view, before which it runs no script: qtmozembed
    // refuses one, with a warning (QMozViewPrivate::runJavaScript).
    property bool ready: false

    // As early as the view says anything of a new document -- its address -- and again
    // once it has loaded, for a document the first try came too early for. A page that
    // has it already is left as it is.
    function install() {
        if (ready) {
            view.runJavaScript(WebNotifications.pageScript)
        }
    }

    // A Connections takes every handler in it for its target's, so each is held here
    // rather than being this object.
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
        // The platform's own answer to a page that asked the engine rather than this
        // browser, which WebNotifications takes back.
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

    // Both before the view has loaded anything: what the frame script says is heard only
    // once asked for, and qtmozembed keeps a frame script until the engine has made the
    // view, which then has it before its first page.
    Component.onCompleted: {
        view.addMessageListener(WebNotifications.messageName)
        view.loadFrameScript(WebNotifications.relayScriptUrl)
    }
    // The view goes with its page: the tab closed, or its view given up for another's
    // (docs/DECISIONS/0016-five-live-pages.md).
    Component.onDestruction: WebNotifications.forgetTab(pageTabId)
}
