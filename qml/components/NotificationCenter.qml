// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Web notifications: Nemo Notification per WebNotifications key, permission prompts, and
// single entry point for engine permission answers (NotificationPermissions, SitePermissions).
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import Nemo.Notifications 1.0
import harbour.salama 1.0

QtObject {
    id: center

    property Item page
    property var shown: ({})
    // Installed path (rpm/harbour-salama.spec).
    readonly property string appIcon:
        Qt.resolvedUrl("../../../icons/hicolor/172x172/apps/harbour-salama.png")

    property Component notificationComponent: Component {
        Notification {
            property int key: 0

            objectName: "webNotification"
            appName: "Salama"
            appIcon: center.appIcon
            // No D-Bus call named -> platform tells this process.
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

    // Swiped away or platform-closed.
    function gone(key) {
        var notification = shown[key]
        if (notification) {
            delete shown[key]
            notification.destroy()
            WebNotifications.closed(key)
        }
    }

    // Ask only while page on screen with nothing over it, else refuse this time.
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

    function withdraw(tabId) {
        var top = pageStack.currentPage
        if (top && top.objectName === "notificationPermissionDialog" && top.tabId === tabId) {
            pageStack.pop()
        }
    }

    function tellEngine(topic, payload) {
        WebEngine.notifyObservers(topic, payload)
    }

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
        onRecvObserve: {
            NotificationPermissions.observe(message, data)
            SitePermissions.observe(message, data)
        }
    }

    property Connections settings: Connections {
        target: PrivacySettings
        onBlockNotificationRequestsChanged: center.applyRequests()
    }

    Component.onCompleted: {
        // Not via Connections, so connected before first request.
        NotificationPermissions.engineRequest.connect(center.tellEngine)
        SitePermissions.engineRequest.connect(center.tellEngine)
        WebEngine.addObserver(NotificationPermissions.topic)
        NotificationPermissions.refresh()
        applyRequests()
        // Close leftovers from unclean exit: their pages are gone.
        var probe = notificationComponent.createObject(center)
        var left = probe.notifications()
        for (var i = 0; i < left.length; ++i) {
            left[i].close()
        }
        probe.destroy()
    }
}
