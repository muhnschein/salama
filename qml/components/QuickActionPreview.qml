// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover as the home screen draws it, small: one of the two pictures the cover's
// settings page offers to choose what the cover shows (pages/CoverSettingsPage.qml,
// docs/DECISIONS/0031-cover-is-lightning.md), each with the quick action where the home
// screen draws it, alone in the middle of the strip along the foot
// (0029-quick-action.md). While a tab plays the tab's mute is drawn beside it (0026);
// the page says so in a line rather than in a second pair of pictures.
//
// What is behind the actions is the cover as it is set to show itself. The lightning is
// the cover's own, at rest (components/CoverLightning.qml); the heading over the tab last
// read is drawn as where things go rather than as what they are: bars for the words, a
// blank cell for the page. Every measure is the cover's own (cover/CoverPage.qml) scaled
// from a real cover's size, so the pictures the actions wear are as large against it as
// they will be there, and they are the very files the cover hands the home screen. Not a
// button itself: the page makes each picture a choice, ringed while it is the one set,
// and the pictures follow the quick action chosen under them.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // The glyph the action wears, by the name CoverSettings.iconPath takes; "" for none.
    property string glyph
    // Whether the ambience is dark, which is the ink the pictures are drawn in.
    property bool onDark: true
    // What the picture shows, under it.
    property alias text: caption.text
    // Which cover it is a picture of, a CoverSettings.Style value, and whether it is the
    // one set.
    property int style: CoverSettings.style
    property bool selected

    // How much smaller than a real cover the picture is.
    readonly property real ratio: width / Theme.coverSizeLarge.width
    readonly property real iconSize: Theme.iconSizeSmall * ratio
    readonly property bool latestTab: style === CoverSettings.LatestTab
    // The action along the foot, or none.
    readonly property var actions: glyph === "" ? [] : [glyph]

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

        // The ring, over the picture's edge: the highlight while it is the cover set.
        Rectangle {
            objectName: "previewRing"
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: preview.selected ? 2 * Theme._lineWidth : Theme._lineWidth
            border.color: preview.selected ? Theme.highlightColor
                                           : Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
        }
    }

    Label {
        id: caption

        y: picture.height + Theme.paddingSmall
        width: preview.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        font.pixelSize: Theme.fontSizeSmall
        color: preview.selected ? Theme.highlightColor : Theme.primaryColor
    }
}
