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
// What is behind the actions is the cover as it is set to show itself -- the app's mark
// alone, or the heading over the tab last read or over the most recent tabs -- drawn as
// where things go rather than as what they are: bars for the words, blank cells for the
// pages. Every measure is the cover's own (cover/CoverPage.qml) scaled from a real cover's
// size, so the pictures the actions wear are as large against it as they will be there,
// and they are the very files the cover hands the home screen. Not a button: the picture
// only follows the choices under it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // The glyph the action wears, by the name Settings.coverIconPath takes; "" for none.
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
    readonly property bool iconOnly: Settings.coverStyle === Settings.CoverIconOnly
    readonly property bool everyTab: Settings.coverStyle === Settings.CoverEveryTab
    // The actions along the foot, left to right: the one alone, or the action and the
    // mute beside it, or the mute alone. "" is the place of an action not chosen.
    readonly property var actions: !playing ? [glyph] : (glyph === "" ? ["speaker-on"]
                                                                      : [glyph, "speaker-on"])

    // A file of the cover's own, as a whole URL, resolved from here as the cover
    // resolves it; nothing for no glyph.
    function iconSource(name) {
        if (name === "") {
            return ""
        }
        return Qt.resolvedUrl("../../" + Settings.coverIconPath(name, preview.iconSize,
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

        // The icon-only cover: the app's own mark, as large against the cover as there.
        Image {
            anchors.centerIn: parent
            width: Math.round(parent.width * 0.45)
            height: width
            visible: preview.iconOnly
            opacity: Theme.opacityHigh
            smooth: true
            source: Qt.resolvedUrl("../../art/harbour-salama.png")
        }

        // The heading: the name and what the number counts, top left, and the number top
        // right, each a bar the height of its letters.
        Column {
            id: heading

            x: Theme.paddingLarge * preview.ratio
            y: x
            visible: !preview.iconOnly
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
            visible: !preview.iconOnly
            width: Theme.fontSizeHuge * preview.ratio / 2
            height: Theme.fontSizeHuge * preview.ratio * 0.7
            radius: Theme.paddingSmall * preview.ratio
            color: Theme.primaryColor
            // Quieter than the cover's own number: what the picture is about is the actions.
            opacity: Theme.opacityHigh
        }

        // The tabs, grey and half there: the one last read across the whole of the room,
        // or the most recent in two columns.
        Grid {
            id: field

            readonly property real gap: Theme.paddingSmall * preview.ratio
            readonly property int rows: preview.everyTab ? 3 : 1

            x: Theme.paddingMedium * preview.ratio
            y: heading.y + heading.height + Theme.paddingLarge * preview.ratio
            width: picture.width - 2 * x
            height: picture.height - y - x
            visible: !preview.iconOnly
            opacity: Theme.opacityLow
            columns: preview.everyTab ? 2 : 1
            spacing: gap

            Repeater {
                model: field.rows * field.columns

                Rectangle {
                    width: (field.width - (field.columns - 1) * field.gap) / field.columns
                    height: (field.height - (field.rows - 1) * field.gap) / field.rows
                    radius: field.gap
                    color: Theme.secondaryColor
                }
            }
        }

        // Where the home screen draws the actions: across the strip along the foot, a
        // small item tall, alone in the middle or one in the middle of each half.
        Repeater {
            model: preview.actions

            Item {
                // The glyph drawn here, and the file it is drawn from; "" for the dot.
                readonly property string glyph: modelData
                readonly property string source: preview.iconSource(modelData)

                objectName: "previewAction"
                x: picture.width * (2 * index + 1) / (2 * preview.actions.length) - width / 2
                y: picture.height - (Theme.itemSizeSmall * preview.ratio + height) / 2
                width: preview.iconSize
                height: width

                Image {
                    anchors.fill: parent
                    visible: parent.source !== ""
                    source: parent.source
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                Rectangle {
                    anchors.centerIn: parent
                    visible: parent.source === ""
                    width: Math.round(parent.width * 0.4)
                    height: width
                    radius: width / 2
                    color: Theme.secondaryColor
                }
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
