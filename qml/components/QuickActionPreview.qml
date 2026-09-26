// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover as the home screen draws it, small: a picture on the cover's settings page of
// where its quick action goes (pages/CoverSettingsPage.qml,
// docs/DECISIONS/0029-quick-action.md). With nothing playing the action is alone, in the
// middle of the strip along the foot; while a tab plays the home screen draws two, one in
// the middle of each half, and the tab's mute is the second (0026). With no action chosen
// the mute is alone while a tab plays, and a dot keeps the action's place while nothing
// does.
//
// What is behind the actions is the cover as it is set to show itself. The lightning is
// the cover's own, at rest (components/CoverLightning.qml); the heading over the tab last
// read is drawn as where things go rather than as what they are: bars for the words, a
// blank cell for the page. Every measure is the cover's own (cover/CoverPage.qml) scaled
// from a real cover's size, so the pictures the actions wear are as large against it as
// they will be there, and they are the very files the cover hands the home screen. Not a
// button: the picture only follows the choices under it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // The glyph the action wears, by the name CoverSettings.iconPath takes; "" for none.
    property string glyph
    // Drawn as it is while a tab plays: with the mute.
    property bool playing: false
    // Whether the ambience is dark, which is the ink the pictures are drawn in.
    property bool onDark: true
    // What the picture shows, under it.
    property alias text: caption.text

    // How much smaller than a real cover the picture is.
    readonly property real ratio: width / Theme.coverSizeLarge.width
    readonly property real iconSize: Theme.iconSizeSmall * ratio
    readonly property bool latestTab: CoverSettings.style === CoverSettings.LatestTab
    // The actions along the foot, left to right: the one alone, or the action and the
    // mute beside it, or the mute alone, or none.
    readonly property var actions: {
        var drawn = glyph === "" ? [] : [glyph]
        return playing ? drawn.concat(["speaker-on"]) : drawn
    }

    // A file of the cover's own, as a whole URL, resolved from here as the cover
    // resolves it.
    function iconSource(name) {
        return Qt.resolvedUrl("../../" + CoverSettings.iconPath(name, preview.iconSize,
                                                                 preview.onDark))
    }

    height: picture.height + Theme.paddingSmall + caption.height

    Rectangle {
        id: picture

        objectName: "previewPicture"
        width: preview.width
        height: Math.round(width * Theme.coverSizeLarge.height / Theme.coverSizeLarge.width)
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityHigh)
        clip: true

        // The lightning, as the cover draws it, and still: the flash is for the home
        // screen, not for a page of settings.
        CoverLightning {
            objectName: "previewLightning"
            anchors.fill: parent
            visible: !preview.latestTab
            onDark: preview.onDark
        }

        // The heading: the name and what the number counts, top left, and the number top
        // right, each a bar the height of its letters.
        Column {
            id: heading

            x: Theme.paddingLarge * preview.ratio
            y: x
            visible: preview.latestTab
            spacing: Theme.paddingSmall * preview.ratio

            Rectangle {
                width: picture.width * 0.4
                height: Theme.fontSizeMedium * preview.ratio / 2
                radius: height / 2
                color: Theme.highlightColor
            }

            Rectangle {
                width: picture.width * 0.2
                height: Theme.fontSizeExtraSmall * preview.ratio / 2
                radius: height / 2
                color: Theme.secondaryHighlightColor
            }
        }

        Rectangle {
            anchors {
                top: parent.top
                right: parent.right
                topMargin: Theme.paddingMedium * preview.ratio
                rightMargin: Theme.paddingLarge * preview.ratio
            }
            visible: preview.latestTab
            width: Theme.fontSizeHuge * preview.ratio / 2
            height: Theme.fontSizeHuge * preview.ratio * 0.7
            radius: Theme.paddingSmall * preview.ratio
            color: Theme.primaryColor
            // Quieter than the cover's own number: what the picture is about is the actions.
            opacity: Theme.opacityHigh
        }

        // The tab last read, grey and half there, across the whole of the room.
        Rectangle {
            x: Theme.paddingMedium * preview.ratio
            y: heading.y + heading.height + Theme.paddingLarge * preview.ratio
            width: picture.width - 2 * x
            height: picture.height - y - x
            visible: preview.latestTab
            opacity: Theme.opacityLow
            radius: Theme.paddingSmall * preview.ratio
            color: Theme.secondaryColor
        }

        // Where the home screen draws the actions: across the strip along the foot, a
        // small item tall, alone in the middle or one in the middle of each half.
        Repeater {
            model: preview.actions

            Image {
                // The glyph drawn here; the file it is drawn from is the source.
                readonly property string glyph: modelData

                objectName: "previewAction"
                x: picture.width * (2 * index + 1) / (2 * preview.actions.length) - width / 2
                y: picture.height - (Theme.itemSizeSmall * preview.ratio + height) / 2
                width: preview.iconSize
                height: width
                source: preview.iconSource(modelData)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
        }
    }

    Label {
        id: caption

        y: picture.height + Theme.paddingSmall
        width: preview.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        font.pixelSize: Theme.fontSizeExtraSmall
        color: Theme.secondaryHighlightColor
    }
}
