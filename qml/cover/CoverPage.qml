// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
// Downloads win over media. No tab or start page: bolt alone, full strength.
// Static: redrawn only on content change.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

CoverBackground {
    id: cover

    readonly property bool showsMute: TabModel.activeMediaState !== TabModel.NoMedia
                                      || TabModel.activeMuted
    readonly property bool heard: TabModel.activeMediaState === TabModel.MediaPlaying
                                  && !TabModel.activeMuted
    readonly property bool showsDownloads: DownloadModel.runningCount > 0
    readonly property bool showsMedia: cover.showsMute && !cover.showsDownloads
    readonly property bool showsPlace: !cover.showsDownloads && !cover.showsMedia
                                       && TabModel.count > 0 && TabModel.activeUrl.length > 0
    readonly property bool showsQuickAction: CoverSettings.quickAction !== CoverSettings.QuickActionNone

    readonly property bool onDark: {
        var ink = Theme.primaryColor
        return 0.299 * ink.r + 0.587 * ink.g + 0.114 * ink.b > 0.5
    }

    /// Home screen draws file as-is, so pre-rendered at Silica small icon size in ambience
    /// ink (icons/render.sh). Not theme icon-cover-* glyphs: can't match bar's speaker.
    function actionIcon(glyph) {
        return Qt.resolvedUrl("../../" + CoverSettings.iconPath(glyph, Theme.iconSizeSmall,
                                                                 cover.onDark))
    }

    readonly property string quickActionIcon: {
        var glyphs = ["", "search", "bookmarks", CoverSettings.quickActionIcon, "downloads", "history"]
        return cover.showsQuickAction ? cover.actionIcon(glyphs[CoverSettings.quickAction]) : ""
    }
    readonly property string muteIcon: cover.actionIcon(cover.heard ? "speaker-on"
                                                                    : "speaker-mute")

    objectName: "coverPage"

    // Declared first to sit under all.
    CoverHalftone {
        anchors.fill: parent
        visible: !(cover.showsMedia && media.pictured)
        // Theme's faintest opacity still pulled eye off title.
        strength: cover.showsPlace || cover.showsDownloads || cover.showsMedia ? 0.12 : 1
    }

    Item {
        anchors {
            fill: parent
            margins: Theme.paddingLarge
            bottomMargin: Theme.itemSizeSmall
        }

        CoverPlace {
            anchors.fill: parent
            visible: cover.showsPlace
            url: TabModel.activeUrl
            title: TabModel.activeTitle
            favicon: TabModel.activeFavicon
        }

        CoverDownloads {
            anchors.fill: parent
            visible: cover.showsDownloads
            count: DownloadModel.runningCount
            progress: DownloadModel.runningProgress
        }

        CoverMedia {
            id: media

            anchors.fill: parent
            visible: cover.showsMedia
            heard: cover.heard
            title: TabModel.activeMediaTitle
            artist: TabModel.activeMediaArtist
            artwork: TabModel.activeMediaArtwork
            pageTitle: TabModel.activeTitle
            url: TabModel.activeUrl
        }
    }

    // Action handling lives in harbour-salama.qml: page stack out of cover scope.
    // Home screen draws only enabled list, so one list per combo.
    CoverActionList {
        objectName: "quickCoverActions"
        enabled: cover.showsQuickAction && !cover.showsMute

        CoverAction {
            objectName: "quickCoverAction"
            iconSource: cover.quickActionIcon
            onTriggered: window.quickAction()
        }
    }

    CoverActionList {
        objectName: "mediaCoverActions"
        enabled: cover.showsQuickAction && cover.showsMute

        CoverAction {
            objectName: "mediaQuickCoverAction"
            iconSource: cover.quickActionIcon
            onTriggered: window.quickAction()
        }

        CoverAction {
            objectName: "muteCoverAction"
            iconSource: cover.muteIcon
            onTriggered: PageMedia.toggleMuted(TabModel.activeTabId)
        }
    }

    CoverActionList {
        objectName: "muteCoverActions"
        enabled: !cover.showsQuickAction && cover.showsMute

        CoverAction {
            objectName: "loneMuteCoverAction"
            iconSource: cover.muteIcon
            onTriggered: PageMedia.toggleMuted(TabModel.activeTabId)
        }
    }
}
