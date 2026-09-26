// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// What the cover shows while the app is minimised. By default nothing to read: the
// bolt, with a flash of sheet lightning each time it comes into view
// (components/CoverLightning.qml, docs/DECISIONS/0031-cover-is-lightning.md). A reader
// who wants the cover to say something can have the last tab instead, from Settings:
// how many tabs are open, over the one last in front.
//
// Either way the cover's actions are what it offers: the one quick action chosen in
// Settings (docs/DECISIONS/0029-quick-action.md), and while the tab in front plays
// something its mute beside it (docs/DECISIONS/0026-media-controls.md).
//
// The last tab's heading is laid out as the platform's own covers lay theirs out, and
// with postivene's and vuo's measures exactly: the name top left with a line under it,
// the two set closer than their line boxes would put them, and the number top right,
// large. Under it, the tab itself, grey and half there, fading in a large padding
// below the heading -- see components/CoverTabPicture.qml.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0
import "../components"

CoverBackground {
    id: cover

    /// The app's own name, and never a translated one: a name is not a word to be
    /// put into another language. Held as a property rather than written into the
    /// label so that ci/qml-lint.sh's untranslated-string check stays as strict as
    /// it is -- every bare string in a text: binding is a defect, and this is the one
    /// string that is not.
    readonly property string brandName: "Salama"

    /// What the cover is set to show (CoverSettings.style): the lightning, or the
    /// heading over the tab last read. Everything below turns on this.
    readonly property bool showsLatestTab: CoverSettings.style === CoverSettings.LatestTab

    /// The front tab plays something, or is muted: its mute is offered, as the bar
    /// offers it (components/AddressLabel.qml).
    readonly property bool showsMute: TabModel.activeMediaState !== TabModel.NoMedia
                                      || TabModel.activeMuted
    /// A quick action is chosen, which it is unless the reader chose none.
    readonly property bool showsQuickAction: CoverSettings.quickAction !== CoverSettings.QuickActionNone

    /// Whether the ambience is a dark one -- its ink, the primary colour, is light --
    /// which the actions' pictures are drawn in white for, and in black on a light one,
    /// and which the lightning's flash is lit for.
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
    readonly property string muteIcon: {
        var heard = TabModel.activeMediaState === TabModel.MediaPlaying && !TabModel.activeMuted
        return cover.actionIcon(heard ? "speaker-on" : "speaker-mute")
    }

    objectName: "coverPage"

    // The cover as it is unless Settings says otherwise. It flashes as the cover comes
    // into view, and only while it is the cover being shown.
    CoverLightning {
        anchors.fill: parent
        visible: !cover.showsLatestTab
        // Not on visible: that is whether every item above this one is shown too, and
        // how the home screen holds a cover it draws is its own business.
        active: !cover.showsLatestTab && cover.status === Cover.Active
        onDark: cover.onDark
    }

    // Under the words, and declared before them so that it is: the picture is the
    // ground the heading and the number are read against.
    CoverTabPicture {
        anchors {
            top: heading.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: Theme.paddingLarge
        }
        // Texture, not a picture to be looked into. Grey at full strength reads as
        // a second screen inside the cover and pulls the eye off the number.
        visible: cover.showsLatestTab
        opacity: Theme.opacityLow
        fadeHeight: cover.height * 0.14
        // The tab most recently in front, which is where the reader has just been.
        source: {
            var shots = TabModel.recentThumbnails
            return shots.length > 0 ? shots[0] : ""
        }
    }

    // The name and what the number counts, top left; the number top right, always
    // -- a browser with one tab has something to say as much as one with twelve.
    Column {
        id: heading

        objectName: "coverHeading"
        visible: cover.showsLatestTab
        anchors {
            top: parent.top
            left: parent.left
            right: tabCount.left
            margins: Theme.paddingLarge
            rightMargin: Theme.paddingMedium
        }
        // The two lines are one heading: set closer than their own leading would
        // put them, as postivene's and vuo's are.
        spacing: -Theme.paddingSmall

        Label {
            objectName: "coverBrand"
            width: parent.width
            textFormat: Text.PlainText
            text: cover.brandName
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeMedium
            truncationMode: TruncationMode.Fade
        }

        Label {
            objectName: "coverSubtitle"
            width: parent.width
            textFormat: Text.PlainText
            text: qsTr("Tabs")
            color: Theme.secondaryHighlightColor
            font.pixelSize: Theme.fontSizeExtraSmall
            truncationMode: TruncationMode.Fade
        }
    }

    Label {
        id: tabCount

        objectName: "coverTabCount"
        visible: cover.showsLatestTab
        anchors {
            top: parent.top
            right: parent.right
            topMargin: Theme.paddingMedium
            rightMargin: Theme.paddingLarge
        }
        textFormat: Text.PlainText
        // Two digits is what a browser needs. Past a hundred tabs the number has
        // stopped being a count and become a state of affairs, and the reader is
        // not totting them up off a cover anyway.
        text: TabModel.count > 99 ? "99+" : TabModel.count
        // Three glyphs at the huge size run straight over the app's name; the
        // number is anchored to the edge and grows leftwards into it. It steps
        // down instead, which keeps the digits legible AND the name readable.
        font.pixelSize: TabModel.count > 99 ? Theme.fontSizeExtraLarge : Theme.fontSizeHuge
        color: Theme.primaryColor
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
