// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What each kind of site permission, and each choice about one, is called, in one place:
// the page of defaults, the page of a kind's exceptions and the details of one site all
// say the same of the same thing (docs/DECISIONS/0039-site-permissions.md), as
// SettingNames.qml has the settings' choices said (0036-settings-choose-in-place.md).
// A kind is a SitePermissions kind; a decision, one of its decisions.
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

    // sailfish-browser's own for the five it has
    // (apps/browser/qml/pages/components/CertificateInfo.qml), the platform's for
    // notifications and, for tracking protection, the one Settings draws it with.
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

    // What a kind is set to for the sites that have no exception to it.
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

    // Whether what a kind is set to for the sites with no exception is to be asked: the
    // one choice a site cannot be given of its own, since a site with no record is what
    // asking is.
    function defaultAsks(kind) {
        switch (kind) {
        case SitePermissions.Notifications:
            return !PrivacySettings.blockNotificationRequests
        case SitePermissions.Location:
            return !SitePermissionSettings.locationBlocked
        case SitePermissions.Camera:
            return !SitePermissionSettings.cameraBlocked
        case SitePermissions.Microphone:
            return !SitePermissionSettings.microphoneBlocked
        }
        return false
    }

    // What a site was given, as the details of one say it.
    function decisionName(decision) {
        return decision === SitePermissions.Allow
                //: A site has been allowed it
                ? qsTr("Allowed")
                //: A site has been blocked from it
                : qsTr("Blocked")
    }

    // How many exceptions there are, as the page of defaults says it under each.
    function exceptionCount(count) {
        //: Under a kind of permission, when no site has been given an exception to it
        return count === 0 ? qsTr("No exceptions") : qsTr("%n exception(s)", "", count)
    }
}
