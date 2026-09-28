// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// The cover as the home screen draws it, small, with the quick action where the home
// screen draws it, alone in the middle of the strip along the foot: the picture at the
// head of the cover's settings page (pages/CoverSettingsPage.qml, 0029-quick-action.md).
// While a tab plays the tab's mute is drawn beside it (0026); the page says so in a line
// rather than in a second picture.
//
// What is behind the action is the cover with nothing to say, the halftone alone
// (components/CoverHalftone.qml, docs/DECISIONS/0037-cover-is-where-you-were.md). Every
// measure is the cover's own (cover/CoverPage.qml) scaled from a real cover's size, so
// the picture the action wears is as large against it as it will be there, and it is the
// very file the cover hands the home screen. The picture follows the quick action chosen
// under it.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // The glyph the action wears, by the name CoverSettings.iconPath takes; "" for none.
    property string glyph
    // Whether the ambience is dark, which is the ink the pictures are drawn in.
    property bool onDark: true

    // How much smaller than a real cover the picture is.
    readonly property real ratio: width / Theme.coverSizeLarge.width
    readonly property real iconSize: Theme.iconSizeSmall * ratio
    // The action along the foot, or none.
    readonly property var actions: glyph === "" ? [] : [glyph]

    // A file of the cover's own, as a whole URL, resolved from here as the cover
    // resolves it.
    function iconSource(name) {
        return Qt.resolvedUrl("../../" + CoverSettings.iconPath(name, preview.iconSize,
                                                                 preview.onDark))
    }

    height: picture.height

    Rectangle {
        id: picture

        objectName: "previewPicture"
        width: preview.width
        height: Math.round(width * Theme.coverSizeLarge.height / Theme.coverSizeLarge.width)
        radius: Theme.paddingSmall
        color: Theme.rgba(Theme.highlightDimmerColor, Theme.opacityHigh)
        border.width: Theme._lineWidth
        border.color: Theme.rgba(Theme.primaryColor, Theme.opacityFaint)
        clip: true

        CoverHalftone {
            objectName: "previewHalftone"
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
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
}
