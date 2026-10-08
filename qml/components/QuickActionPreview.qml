// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 salama contributors
//
// Measures from cover/CoverPage.qml scaled by real cover size.
import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.salama 1.0

Item {
    id: preview

    // CoverSettings.iconPath name; "" for none.
    property string glyph
    property bool onDark: true

    readonly property real ratio: width / Theme.coverSizeLarge.width
    readonly property real iconSize: Theme.iconSizeSmall * ratio
    readonly property var actions: glyph === "" ? [] : [glyph]

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
            anchors.fill: parent
        }

        // Home screen layout: centred, or one per half.
        Repeater {
            model: preview.actions

            Image {
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
