// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Sent at browsing page creation (engine queues until up) and on each setting or ambience
// change. Only browsing page creates it: it imports Sailfish.WebEngine.
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebEngine 1.0
import harbour.salama 1.0

QtObject {
    id: preferences

    readonly property bool darkAmbience: Reader.isDarkAmbience(Theme.primaryColor)
    // Last colours sent, so ambience settling at startup doesn't send twice.
    property var givenColors: null

    function give(list) {
        for (var i = 0; i < list.length; ++i) {
            WebEngineSettings.setPreference(list[i].name, list[i].value)
        }
    }

    // Cookies follow tracking protection while on, user choice while off: either change resends.
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
        give(EngineMessages.contentPreferences(PrivacySettings.globalPrivacyControl,
                                               PrivacySettings.javascript))
    }

    function applyHttpsOnly() {
        give(EngineMessages.httpsOnlyPreferences(PrivacySettings.httpsOnly))
    }

    function applyDoh() {
        give(EngineMessages.dohPreferences(DohSettings.protection, DohSettings.provider,
                                           DohSettings.exceptions))
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
        onGlobalPrivacyControlChanged: preferences.applyContent()
        onJavascriptChanged: preferences.applyContent()
        onHttpsOnlyChanged: preferences.applyHttpsOnly()
    }

    property Connections doh: Connections {
        target: DohSettings
        onProtectionChanged: preferences.applyDoh()
        onProviderChanged: preferences.applyDoh()
        onExceptionsChanged: preferences.applyDoh()
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
        applyHttpsOnly()
        applyDoh()
    }
}
