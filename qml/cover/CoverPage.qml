// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the cover shows while the app is minimised: the halftone bolt, and over it, faint,
// what the browser has to say (docs/DECISIONS/0037-cover-is-where-you-were.md).
//
//  * At rest, where the reader was: the site and title of the tab in front, its group and
//    how many tabs are open (components/CoverPlace.qml).
//  * While something downloads, how far it has come (components/CoverDownloads.qml).
//  * While the tab in front plays, or is muted, what plays (components/CoverMedia.qml).
//    Downloads come first when both happen at once.
//  * With no tab open, or the one in front on the start page, the halftone alone, at full
//    strength: there is nowhere to say the reader was.
//
// Either way the cover's actions are what it offers: the one quick action chosen in
// Settings (docs/DECISIONS/0029-quick-action.md), and while the tab in front plays
// something its mute beside it (docs/DECISIONS/0026-media-controls.md).
//
// Nothing on it moves. It is drawn again as what it says changes, and not in between.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

CoverBackground {
    id: cover

    /// The front tab plays something, or is muted: its mute is offered, as the bar
    /// offers it (components/AddressLabel.qml).
    readonly property bool showsMute: TabModel.activeMediaState !== TabModel.NoMedia
                                      || TabModel.activeMuted
    /// It plays and is not muted: the speaker, and "Playing"; otherwise struck through,
    /// and "Paused".
    readonly property bool heard: TabModel.activeMediaState === TabModel.MediaPlaying
                                  && !TabModel.activeMuted
    /// Something downloads.
    readonly property bool showsDownloads: DownloadModel.runningCount > 0
    /// What the tab in front plays, while it plays and nothing downloads.
    readonly property bool showsMedia: cover.showsMute && !cover.showsDownloads
    /// Where the reader was: the tab in front has a page, and nothing else is to be said.
    readonly property bool showsPlace: !cover.showsDownloads && !cover.showsMedia
                                       && TabModel.count > 0 && TabModel.activeUrl.length > 0
    /// A quick action is chosen, which it is unless the reader chose none.
    readonly property bool showsQuickAction: CoverSettings.quickAction !== CoverSettings.QuickActionNone

    /// Whether the ambience is a dark one -- its ink, the primary colour, is light --
    /// which the actions' pictures are drawn in white for, and in black on a light one.
    readonly property bool onDark: {
        var ink = Theme.primaryColor
        return 0.299 * ink.r + 0.587 * ink.g + 0.114 * ink.b > 0.5
    }

    /// An action's picture, as a whole URL. The home screen draws an action's picture
    /// itself, from the file as it is, so it is one drawn at the size Silica's small
    /// icon takes on this phone, in the ambience's ink -- see icons/render.sh, which
    /// draws them from icons/cover/, and CoverSettings.iconPath, which names them. Drawn
    /// here rather than taken from the theme's icon-cover-* glyphs, which could not be
    /// checked against the bar's speaker, nor made for a bookmark of the reader's own.
    function actionIcon(glyph) {
        return Qt.resolvedUrl("../../" + CoverSettings.iconPath(glyph, Theme.iconSizeSmall,
                                                                 cover.onDark))
    }

    /// The quick action's picture: the glyph of what it opens, and for one bookmark the
    /// glyph picked for it in Settings. None for no action, when no list offers it.
    readonly property string quickActionIcon: {
        var glyphs = ["", "search", "bookmarks", CoverSettings.quickActionIcon, "downloads", "history"]
        return cover.showsQuickAction ? cover.actionIcon(glyphs[CoverSettings.quickAction]) : ""
    }
    /// The mute's picture: the speaker while the tab is heard, struck through while it is
    /// not, as on the bar.
    readonly property string muteIcon: cover.actionIcon(cover.heard ? "speaker-on"
                                                                    : "speaker-mute")

    objectName: "coverPage"

    // Under everything, and declared first so that it is. Faint under words, whole when
    // there are none; not under a picture of what plays, which has the room to itself.
    CoverHalftone {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        visible: !(cover.showsMedia && media.pictured)
        // Faint enough that the words read first: Theme's faintest opacity left the bolt
        // pulling the eye off the title.
        strength: cover.showsPlace || cover.showsDownloads || cover.showsMedia ? 0.12 : 1
    }

    // What the cover says, clear of its edges and of the actions along its foot.
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
            group: TabModel.currentGroupName
            tabCount: TabModel.count
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

    // The quick action, alone while nothing plays. What it does is the window's
    // (harbour-salama.qml): the bar, the page stack and the pages it opens belong to the
    // browsing page and the window, which are not in a cover's scope.
    //
    // While the tab in front plays, the quick action and the tab's mute beside it; with
    // no quick action, the mute alone; with neither, no action at all. The home screen
    // draws the one list that is enabled, so there is one for each.
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
