// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the settings ask of the engine through its preferences: the tracking protection
// level (docs/DECISIONS/0023-tracking-protection.md), the colours pages are asked to
// draw themselves in (0035-website-colours.md), and Privacy's Do not track and
// JavaScript switches. Given as the browsing page is made, which
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

    function applyTrackingProtection() {
        give(EngineMessages.trackingProtectionPreferences(PrivacySettings.trackingProtection))
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

    property Connections general: Connections {
        target: Settings
        onWebsiteColorsChanged: preferences.applyWebsiteColors()
    }

    Component.onCompleted: {
        applyTrackingProtection()
        applyWebsiteColors()
        applyContent()
    }
}
