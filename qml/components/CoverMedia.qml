// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover while the tab in front plays something, or is muted: what plays, as the page
// says it (docs/DECISIONS/0037-cover-is-where-you-were.md).
//
// With a picture -- the page's Media Session artwork, or the poster of a video it plays
// (PageMedia) -- the picture across the top, square or wide as it is drawn, and under it
// the title, the artist and whether it plays, and where. Without one, the words alone,
// over the cover's halftone: whether it plays, the site large, and what the page calls
// what plays, or else its own title. Muted or paused, the picture dims.
//
// The picture is fetched and scaled once, to the width it is shown at, when the page names
// another one; nothing about it moves.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: media

    /// Whether the page plays and is heard, which is what the mute beside it shows.
    property bool heard
    /// What the page says of what plays; any of them may be empty.
    property string title
    property string artist
    property string artwork
    /// The page itself: its title and address.
    property string pageTitle
    property string url

    /// Whether the picture is there to show. The cover shows its halftone only when not.
    readonly property bool pictured: picture.status === Image.Ready
    readonly property string host: SearchSettings.displayAddress(url)
    readonly property string playState: heard
                                    //: On the cover, of what the tab in front plays
                                    ? qsTr("Playing")
                                    //: On the cover, of what the tab in front has muted or paused
                                    : qsTr("Paused")
    readonly property string heading: title.length > 0 ? title : pageTitle

    objectName: "coverMedia"

    // Square, or as wide as a video when the picture is clearly wider than tall; cut to
    // fill either way, and never so tall that the words under it run into the actions.
    Item {
        id: frame

        readonly property bool wide: picture.implicitWidth > picture.implicitHeight * 1.2
        readonly property real room: media.height - caption.height - caption.anchors.topMargin

        objectName: "coverMediaFrame"
        visible: media.pictured
        width: wide ? media.width : Math.min(media.width, room)
        height: wide ? Math.min(width * 9 / 16, room) : width
        clip: true
        opacity: media.heard ? 1 : Theme.opacityLow

        Image {
            id: picture

            objectName: "coverMediaArtwork"
            anchors.fill: parent
            // At the widest it is shown, whichever shape it turns out to be: decoded once.
            sourceSize.width: media.width
            fillMode: Image.PreserveAspectCrop
            smooth: true
            asynchronous: true
            source: media.artwork
        }
    }

    Column {
        id: caption

        objectName: "coverMediaCaption"
        visible: media.pictured
        anchors {
            top: frame.bottom
            left: parent.left
            right: parent.right
            topMargin: Theme.paddingMedium
        }

        Label {
            objectName: "coverMediaTitle"
            width: parent.width
            textFormat: Text.PlainText
            text: media.heading
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.primaryColor
        }

        Label {
            objectName: "coverMediaArtist"
            width: parent.width
            visible: text.length > 0
            textFormat: Text.PlainText
            text: media.artist
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }

        Label {
            objectName: "coverMediaState"
            width: parent.width
            textFormat: Text.PlainText
            //: On the cover, whether what the tab in front plays is heard, and the site:
            //: "Playing · yle.fi"
            text: qsTr("%1 · %2").arg(media.playState).arg(media.host)
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.highlightColor
        }
    }

    // Without a picture: the words alone, the site large.
    Column {
        objectName: "coverMediaPlain"
        visible: !media.pictured
        anchors {
            left: parent.left
            right: parent.right
        }

        Label {
            objectName: "coverMediaPlainState"
            width: parent.width
            textFormat: Text.PlainText
            text: media.playState
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.highlightColor
        }

        Label {
            objectName: "coverMediaPlainHost"
            width: parent.width
            textFormat: Text.PlainText
            text: media.host
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraLarge
            color: Theme.primaryColor
        }

        Label {
            objectName: "coverMediaPlainTitle"
            width: parent.width
            textFormat: Text.PlainText
            text: media.heading
            wrapMode: Text.Wrap
            maximumLineCount: 3
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }

        Label {
            objectName: "coverMediaPlainArtist"
            width: parent.width
            visible: text.length > 0
            textFormat: Text.PlainText
            text: media.artist
            truncationMode: TruncationMode.Fade
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }
}
