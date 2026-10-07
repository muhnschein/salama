// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
import QtQuick 2.6
import harbour.salama 1.0

QtObject {
    function kindName(kind) {
        switch (kind) {
        case SitePermissions.Notifications:
            return qsTr("Notifications")
        case SitePermissions.Popups:
            //: Windows a page opens of its own accord
            return qsTr("Pop-ups")
        case SitePermissions.Cookies:
            return qsTr("Cookies")
        case SitePermissions.Location:
            return qsTr("Location")
        case SitePermissions.Camera:
            return qsTr("Camera")
        case SitePermissions.Microphone:
            return qsTr("Microphone")
        case SitePermissions.TrackingProtection:
            return qsTr("Tracking protection")
        }
        return ""
    }

    function kindIcon(kind) {
        switch (kind) {
        case SitePermissions.Notifications:
            return "image://theme/icon-m-notifications"
        case SitePermissions.Popups:
            return "image://theme/icon-m-browser-popup"
        case SitePermissions.Cookies:
            return "image://theme/icon-m-browser-cookies"
        case SitePermissions.Location:
            return "image://theme/icon-m-browser-location"
        case SitePermissions.Camera:
            return "image://theme/icon-m-browser-camera"
        case SitePermissions.Microphone:
            return "image://theme/icon-m-browser-microphone"
        case SitePermissions.TrackingProtection:
            return "image://theme/icon-m-device-lock"
        }
        return ""
    }

    function allow() {
        //: A site may do it
        return qsTr("Allow")
    }

    function block() {
        //: A site may not do it
        return qsTr("Block")
    }

    function ask() {
        //: A site is asked about it each time it wants to
        return qsTr("Ask")
    }

    function cookies(choice) {
        return [
            //: Every site's cookies are accepted
            qsTr("Allow all"),
            //: Cookies a site sets from inside another site's page are refused
            qsTr("Block cross-site"),
            //: No site's cookies are accepted
            qsTr("Block all")
        ][choice] || ""
    }

    function defaultName(kind) {
        switch (kind) {
        case SitePermissions.Notifications:
            return PrivacySettings.blockNotificationRequests ? block() : ask()
        case SitePermissions.Popups:
            return SitePermissionSettings.popupsAllowed ? allow() : block()
        case SitePermissions.Cookies:
            return cookies(SitePermissionSettings.cookies)
        case SitePermissions.Location:
            return SitePermissionSettings.locationBlocked ? block() : ask()
        case SitePermissions.Camera:
            return SitePermissionSettings.cameraBlocked ? block() : ask()
        case SitePermissions.Microphone:
            return SitePermissionSettings.microphoneBlocked ? block() : ask()
        }
        return ""
    }

    function alwaysAsk() {
        //: A site is asked about it each time it wants it, whatever is set for every site
        return qsTr("Always ask")
    }

    function followDefault(kind) {
        //: A site has no choice of its own and does what every site does; %1 is that
        return qsTr("Follow default: %1").arg(defaultName(kind))
    }

    function decisionName(decision) {
        if (decision === SitePermissions.Ask) {
            return alwaysAsk()
        }
        return decision === SitePermissions.Allow
                //: A site has been allowed it
                ? qsTr("Allowed")
                //: A site has been blocked from it
                : qsTr("Blocked")
    }

    function exceptionCount(count) {
        //: Under a kind of permission, when no site has been given an exception to it
        return count === 0 ? qsTr("No exceptions") : qsTr("%n exception(s)", "", count)
    }
}
