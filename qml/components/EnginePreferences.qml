// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the settings ask of the engine through its preferences: the tracking protection
// level (docs/DECISIONS/0023-tracking-protection.md), the cookies accepted while it is
// off and what sites may do unless told otherwise (0039-site-permissions.md), the
// colours pages are asked to draw themselves in (0035-website-colours.md), and Privacy's
// Do not track and JavaScript switches (0044-sailfish-browser-settings.md). Given as the
// browsing page is made, which
// the engine keeps until it is up, and again whenever a setting changes -- or, for pages
// drawn as the ambience is, the ambience.
//
// Made by the browsing page alone, which is the one that has the engine: it imports
// Sailfish.WebEngine for the preferences, as that page does (docs/ARCHITECTURE.md). What
// each setting stands for in the engine's own words is EngineMessages'.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

QtObject {
    id: preferences

    readonly property bool darkAmbience: Reader.isDarkAmbience(Theme.primaryColor)
    // What pages were last told of the colours, so that the ambience settling as the
    // page is made does not say it twice.
    property var givenColors: null

    function give(list) {
        for (var i = 0; i < list.length; ++i) {
            WebEngineSettings.setPreference(list[i].name, list[i].value)
        }
    }

    // The cookies are tracking protection's own preference while it is on, and the
    // reader's choice while it is off, so a change of either gives them again.
    function applyTrackingProtection() {
        give(EngineMessages.trackingProtectionPreferences(PrivacySettings.trackingProtection,
                                                          SitePermissionSettings.cookies))
    }

    function applySitePermissions() {
        give(EngineMessages.sitePermissionPreferences(SitePermissionSettings.popupsAllowed,
                                                      SitePermissionSettings.locationBlocked,
                                                      SitePermissionSettings.cameraBlocked,
                                                      SitePermissionSettings.microphoneBlocked))
    }

    function applyContent() {
        give(EngineMessages.contentPreferences(PrivacySettings.doNotTrack,
                                               PrivacySettings.javascript))
    }

    function applyWebsiteColors() {
        var list = EngineMessages.websiteColorPreferences(Settings.websiteColors, darkAmbience)
        if (givenColors !== list[0].value) {
            givenColors = list[0].value
            give(list)
        }
    }

    onDarkAmbienceChanged: applyWebsiteColors()

    property Connections privacy: Connections {
        target: PrivacySettings
        onTrackingProtectionChanged: preferences.applyTrackingProtection()
        onDoNotTrackChanged: preferences.applyContent()
        onJavascriptChanged: preferences.applyContent()
    }

    property Connections sites: Connections {
        target: SitePermissionSettings
        onCookiesChanged: preferences.applyTrackingProtection()
        onPopupsAllowedChanged: preferences.applySitePermissions()
        onLocationBlockedChanged: preferences.applySitePermissions()
        onCameraBlockedChanged: preferences.applySitePermissions()
        onMicrophoneBlockedChanged: preferences.applySitePermissions()
    }

    property Connections general: Connections {
        target: Settings
        onWebsiteColorsChanged: preferences.applyWebsiteColors()
    }

    Component.onCompleted: {
        applyTrackingProtection()
        applySitePermissions()
        applyWebsiteColors()
        applyContent()
    }
}
