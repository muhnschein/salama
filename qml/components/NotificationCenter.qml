// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The browser's side of the pages' notifications (docs/DECISIONS/0033-web-notifications.md):
//
//  * what they show, as the platform shows a notification: one of Nemo.Notifications'
//    for each that WebNotifications keeps, made the first time it is shown and published
//    again to show another in its place, as the platform replaces what one published
//    showed. Its title and text are the page's, with the site's host under them, where
//    Firefox for Android names the site; its picture is the page's icon when it has one.
//    A tap on one, or its going, is WebNotifications' to answer;
//  * the question a page asks before it may, put while the page is the one on the
//    screen;
//  * the sites allowed and blocked, which the engine keeps, and whether others may ask.
//
// Made by the browsing page alone, whose page it asks over, and which is the one that
// has the engine: it imports Sailfish.WebEngine for the permissions, as that page does
// (docs/ARCHITECTURE.md).
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import Nemo.Notifications 1.0
import harbour.salama 1.0

QtObject {
    id: center

    // The browsing page.
    property Item page
    // By WebNotifications' key.
    property var shown: ({})
    // The browser's own icon, which the platform may show each one under: the file the
    // package installs, from this file's place in it (rpm/harbour-salama.spec).
    readonly property string appIcon:
        Qt.resolvedUrl("../../../icons/hicolor/172x172/apps/harbour-salama.png")

    property Component notificationComponent: Component {
        Notification {
            property int key: 0

            objectName: "webNotification"
            appName: "Salama"
            appIcon: center.appIcon
            // What a tap on the notification invokes; with no D-Bus call named, the
            // platform tells this process, which is the one that can show the page.
            remoteActions: [{ "name": "default" }]
            onClicked: WebNotifications.activate(key)
            onClosed: center.gone(key)
        }
    }

    function publish(key, fields) {
        var notification = shown[key]
        if (!notification) {
            notification = notificationComponent.createObject(center, { "key": key })
            shown[key] = notification
        }
        notification.summary = fields.summary
        notification.body = fields.body
        notification.previewSummary = fields.summary
        notification.previewBody = fields.body
        notification.subText = fields.subText
        notification.icon = fields.icon
        notification.publish()
    }

    function close(key) {
        var notification = shown[key]
        if (notification) {
            delete shown[key]
            notification.close()
            notification.destroy()
        }
    }

    // Swiped away, or closed by the platform: not by this browser, which forgets one
    // as it closes it.
    function gone(key) {
        var notification = shown[key]
        if (notification) {
            delete shown[key]
            notification.destroy()
            WebNotifications.closed(key)
        }
    }

    // Firefox asks in the tab the page is in, while it is shown; this asks while the
    // page is the one on the screen with nothing over it, and refuses it -- this time --
    // otherwise. The page asked as the reader touched it, so it is almost always so.
    function ask(tabId, host) {
        if (tabId !== TabModel.activeTabId || PageActivity.background
                || pageStack.currentPage !== page || pageStack.busy || page.tabsOpen) {
            WebNotifications.answer(tabId, WebNotifications.NotNow)
            return
        }
        page.uncover()
        pageStack.push(Qt.resolvedUrl("../pages/NotificationPermissionDialog.qml"),
                       { "tabId": tabId, "host": host })
    }

    // The page that asked has gone, and its question with it.
    function withdraw(tabId) {
        var top = pageStack.currentPage
        if (top && top.objectName === "notificationPermissionDialog" && top.tabId === tabId) {
            pageStack.pop()
        }
    }

    // The sites allowed and blocked are the engine's to keep.
    function tellEngine(topic, payload) {
        WebEngine.notifyObservers(topic, payload)
    }

    // Whether sites may ask: the engine's default for the permission.
    function applyRequests() {
        var preference = NotificationPermissions.defaultPreference(PrivacySettings.blockNotificationRequests)
        WebEngineSettings.setPreference(preference.name, preference.value)
    }

    property Connections requests: Connections {
        target: WebNotifications
        onPublishRequested: center.publish(key, fields)
        onCloseRequested: center.close(key)
        onPermissionRequested: center.ask(tabId, host)
        onPermissionWithdrawn: center.withdraw(tabId)
    }

    property Connections engine: Connections {
        target: WebEngine
        onRecvObserve: NotificationPermissions.observe(message, data)
    }

    property Connections settings: Connections {
        target: PrivacySettings
        onBlockNotificationRequestsChanged: center.applyRequests()
    }

    Component.onCompleted: {
        // Connected here rather than by a Connections, so that it is before the first
        // request, which nothing would carry otherwise.
        NotificationPermissions.engineRequest.connect(center.tellEngine)
        WebEngine.addObserver(NotificationPermissions.topic)
        NotificationPermissions.refresh()
        applyRequests()
        // What a browser that stopped without closing them left behind: their pages are
        // gone, and a tap on one would find nothing. The platform lists an application's
        // own; asking goes through a notification, since that is where the call is.
        var probe = notificationComponent.createObject(center)
        var left = probe.notifications()
        for (var i = 0; i < left.length; ++i) {
            left[i].close()
        }
        probe.destroy()
    }
}
